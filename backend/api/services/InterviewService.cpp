#include "../include/services/InterviewService.h"
#include "../include/logger.h"
#include "../include/Constanst.h"
#include "../include/services/TranscriptDocumentBuilder.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <set>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

namespace {

std::optional<std::string> readTextFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return std::nullopt;
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Inicios de segmento de transcript_raw.json (escrito por el job junto a
// transcript_final.txt). Vacio si no existe o no se puede leer: la vista
// funciona igual, solo sin marcas de tiempo.
std::vector<double> readSegmentStarts(const std::filesystem::path& rawJsonPath) {
    std::vector<double> starts;
    std::ifstream file(rawJsonPath);
    if (!file.is_open()) return starts;
    try {
        const auto json = nlohmann::json::parse(file);
        for (const auto& segment : json) {
            starts.push_back(segment.at("start").get<double>());
        }
    } catch (const std::exception& e) {
        log_event(std::string("[InterviewService][readSegmentStarts] No se pudo leer ") + rawJsonPath.string() + ": " + e.what());
        starts.clear();
    }
    return starts;
}

}  // namespace

InterviewService::InterviewService(IInterviewRepository& repository,
                                    hermes::jobs::IInterviewJobRepository& jobRepository,
                                    hermes::jobs::IJobQueue& jobQueue)
    : m_repository(repository), m_jobRepository(jobRepository), m_jobQueue(jobQueue) {}

std::optional<int> InterviewService::createInterview(const std::string& date, const std::string& type, const std::string& subjectType) {
    return m_repository.create(date, type, subjectType);
}

std::vector<InterviewRecord> InterviewService::listInterviews() {
    return m_repository.findAll();
}

std::optional<InterviewDetailRecord> InterviewService::getInterviewDetail(int id) {
    auto interview = m_repository.findById(id);
    if (!interview.has_value()) {
        return std::nullopt;
    }

    InterviewDetailRecord detail;
    detail.interview = interview.value();
    detail.audio = m_repository.findAudioByInterviewId(id);
    detail.transcriptionPath = m_repository.findTranscriptionPathByInterviewId(id);
    detail.summaryPath = m_repository.findSummaryPathByInterviewId(id);
    detail.executionTimeSeconds = m_jobRepository.findLatestExecutionTimeSeconds(id);
    detail.currentStep = m_jobRepository.findCurrentStep(id);
    detail.keywords = m_repository.findKeywordsByInterviewId(id);
    return detail;
}

ProcessOutcome InterviewService::requestProcessing(int id, bool includeSummary, bool enhanceTranscript) {
    if (!m_repository.existsById(id)) {
        return ProcessOutcome::NotFound;
    }

    if (!m_repository.findAudioByInterviewId(id).has_value()) {
        return ProcessOutcome::AudioRequired;
    }

    if (m_jobRepository.hasActiveJob(id)) {
        return ProcessOutcome::AlreadyQueued;
    }

    if (!m_jobRepository.createPending(id).has_value()) {
        log_event("[InterviewService][requestProcessing] Fallo creando el job pending, id=" + std::to_string(id));
        return ProcessOutcome::Failed;
    }

    if (!m_repository.updateStatus(id, "processing")) {
        log_event("[InterviewService][requestProcessing] Fallo actualizando el estado a processing, id=" + std::to_string(id));
        return ProcessOutcome::Failed;
    }

    hermes::jobs::Job job;
    job.interviewId = id;
    job.includeSummary = includeSummary;
    job.enhanceTranscript = enhanceTranscript;
    m_jobQueue.enqueue(std::move(job));

    return ProcessOutcome::Ok;
}

KeywordsOutcome InterviewService::setKeywords(int id, const std::vector<std::string>& keywords) {
    if (!m_repository.existsById(id)) {
        return {KeywordsOutcome::Status::NotFound, "", {}};
    }

    std::vector<std::string> normalized;
    std::set<std::string> seen;  // en minusculas, para deduplicar
    for (const auto& raw : keywords) {
        const size_t first = raw.find_first_not_of(" \t");
        if (first == std::string::npos) continue;  // vacio: se ignora (ej. coma final en el archivo)
        const std::string keyword = raw.substr(first, raw.find_last_not_of(" \t") - first + 1);

        const bool hasControlChar = std::any_of(keyword.begin(), keyword.end(),
                                                [](unsigned char c) { return std::iscntrl(c); });
        if (hasControlChar) {
            return {KeywordsOutcome::Status::Invalid, "El termino '" + keyword + "' contiene caracteres no permitidos", {}};
        }
        if (keyword.size() > Config::MAX_KEYWORD_LENGTH) {
            return {KeywordsOutcome::Status::Invalid,
                    "El termino '" + keyword.substr(0, 30) + "...' supera los " + std::to_string(Config::MAX_KEYWORD_LENGTH) + " caracteres", {}};
        }

        std::string lower = keyword;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
        if (seen.insert(lower).second) {
            normalized.push_back(keyword);
        }
    }

    if (normalized.size() > Config::MAX_KEYWORDS) {
        return {KeywordsOutcome::Status::Invalid,
                "Se admiten hasta " + std::to_string(Config::MAX_KEYWORDS) + " palabras clave (se recibieron " +
                    std::to_string(normalized.size()) + ")", {}};
    }

    if (!m_repository.updateKeywords(id, normalized)) {
        log_event("[InterviewService][setKeywords] Fallo guardando las palabras clave, id=" + std::to_string(id));
        return {KeywordsOutcome::Status::Failed, "", {}};
    }
    return {KeywordsOutcome::Status::Ok, "", normalized};
}

RemoveOutcome InterviewService::removeInterview(int id) {
    if (!m_repository.existsById(id)) {
        return RemoveOutcome::NotFound;
    }

    // Buscar el audio ANTES de borrar la entrevista: interviews_audio se
    // elimina en cascada (ON DELETE CASCADE, SQL/init.sql) apenas se borra
    // la fila padre.
    auto audio = m_repository.findAudioByInterviewId(id);

    if (!m_repository.remove(id)) {
        log_event("[InterviewService][removeInterview] Fallo eliminando la entrevista, id=" + std::to_string(id));
        return RemoveOutcome::Failed;
    }

    // Privacy First: la fila de audio ya se borro de la BD via cascada,
    // pero el archivo real en disco no. Dejarlo huerfano en ./uploads
    // seria retener datos sensibles sin que quede registro de a que
    // entrevista pertenecian. Best-effort: si falla, solo se loguea.
    if (audio.has_value()) {
        std::error_code ec;
        std::filesystem::remove(audio->path, ec);
        if (ec) {
            log_event("[InterviewService][removeInterview] No se pudo borrar el audio en disco: " + audio->path + ". Detalle: " + ec.message());
        }
    }
    // Mismo criterio para las transcripciones/resumen generados.
    removeProcessingOutputs(id);

    return RemoveOutcome::Ok;
}

AttachAudioOutcome InterviewService::canAttachAudio(int interviewId) {
    auto interview = m_repository.findById(interviewId);
    if (!interview.has_value()) {
        return AttachAudioOutcome::NotFound;
    }
    if (interview->status == "processing" || m_jobRepository.hasActiveJob(interviewId)) {
        return AttachAudioOutcome::Busy;
    }
    return AttachAudioOutcome::Ok;
}

AttachAudioOutcome InterviewService::attachAudio(int interviewId, const std::string& path, const std::string& format, long long size) {
    const std::string idTag = " (interview_id=" + std::to_string(interviewId) + ")";

    // Privacy First: un archivo rechazado no puede quedar huerfano en
    // ./uploads (seria audio sensible sin registro de a que pertenece).
    auto discardUpload = [&]() {
        std::error_code ec;
        std::filesystem::remove(path, ec);
        if (ec) {
            log_event("[InterviewService][attachAudio] No se pudo borrar el upload rechazado " + path + ": " + ec.message() + idTag);
        }
    };

    // Se repite el chequeo previo: entre canAttachAudio y este punto pudo
    // haberse encolado un procesamiento.
    const AttachAudioOutcome precheck = canAttachAudio(interviewId);
    if (precheck != AttachAudioOutcome::Ok) {
        discardUpload();
        return precheck;
    }

    const auto previous = m_repository.findAudioByInterviewId(interviewId);
    const bool registered = previous.has_value()
        ? m_repository.updateAudio(interviewId, path, format, size)
        : m_repository.insertAudio(interviewId, path, format, size);
    if (!registered) {
        log_event("[InterviewService][attachAudio] Fallo al registrar el audio en la BD" + idTag);
        discardUpload();
        return AttachAudioOutcome::Failed;
    }

    if (previous.has_value()) {
        // El resultado anterior se genero con el audio viejo: se descarta
        // para no ofrecer una transcripcion que no corresponde.
        if (!m_repository.removeResults(interviewId)) {
            log_event("[InterviewService][attachAudio] No se pudo borrar el resultado anterior de la BD" + idTag);
        }
        removeProcessingOutputs(interviewId);

        std::error_code ec;
        if (previous->path != path) {
            std::filesystem::remove(previous->path, ec);
            if (ec) {
                log_event("[InterviewService][attachAudio] No se pudo borrar el audio anterior " + previous->path + ": " + ec.message() + idTag);
            }
        }
        log_event("[InterviewService][attachAudio] Audio reemplazado; se descarto el resultado anterior" + idTag);
    }

    // No aborta si esto falla (el audio ya quedo guardado y registrado);
    // solo se loguea, igual que el comportamiento previo en FileController.
    if (!m_repository.updateStatus(interviewId, "pending_processing")) {
        log_event("[InterviewService][attachAudio] Fallo actualizando el status de la entrevista a pending_processing" + idTag);
    }

    return AttachAudioOutcome::Ok;
}

void InterviewService::removeProcessingOutputs(int interviewId) {
    const std::filesystem::path dir =
        std::filesystem::path(std::string(Config::STORAGE_DIRECTORY)) / "interviews" / std::to_string(interviewId);
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
    if (ec) {
        log_event("[InterviewService][removeProcessingOutputs] No se pudo borrar " + dir.string() + ": " + ec.message());
    }
}

TranscriptDocumentOutcome InterviewService::getTranscriptDocument(int interviewId) {
    using Status = TranscriptDocumentOutcome::Status;
    if (!m_repository.existsById(interviewId)) {
        return {Status::NotFound, {}};
    }
    const auto transcriptionPath = m_repository.findTranscriptionPathByInterviewId(interviewId);
    if (!transcriptionPath.has_value()) {
        return {Status::NotReady, {}};
    }
    const auto text = readTextFile(*transcriptionPath);
    if (!text.has_value()) {
        log_event("[InterviewService][getTranscriptDocument] No se encontro el archivo " + *transcriptionPath +
                  " (interview_id=" + std::to_string(interviewId) + ")");
        return {Status::FileMissing, {}};
    }

    const auto rawJsonPath = std::filesystem::path(*transcriptionPath).parent_path() / "transcript_raw.json";
    TranscriptDocument document = TranscriptDocumentBuilder::build(*text, readSegmentStarts(rawJsonPath));

    if (const auto summaryPath = m_repository.findSummaryPathByInterviewId(interviewId)) {
        document.summary = readTextFile(*summaryPath);
    }
    return {Status::Ok, std::move(document)};
}

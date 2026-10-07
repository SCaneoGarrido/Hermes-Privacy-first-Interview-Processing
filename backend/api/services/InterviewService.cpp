#include "../include/services/InterviewService.h"
#include "../include/logger.h"
#include "../include/Constanst.h"
#include "../include/services/TranscriptDocumentBuilder.h"
#include "../../transcript/include/TranscriptRenderer.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <set>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

namespace {

using hermes::transcript::SpeakerLabels;
using hermes::transcript::SpeakerSource;
using hermes::transcript::StructuredTranscript;
using hermes::transcript::TranscriptTurn;

// Limites de una transcripcion editada: una entrevista de 2h tiene del orden
// de 3000 segmentos; esto deja margen amplio sin aceptar cualquier cosa.
constexpr size_t MAX_EDIT_BLOCKS = 20000;
constexpr size_t MAX_EDIT_BLOCK_CHARS = 20000;

std::optional<std::string> readTextFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return std::nullopt;
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::vector<TranscriptSpeaker> speakerList(const SpeakerLabels& labels) {
    return {
        {std::string(hermes::transcript::speakerRoleKey(hermes::transcript::SpeakerRole::Interviewer)), labels.interviewer},
        {std::string(hermes::transcript::speakerRoleKey(hermes::transcript::SpeakerRole::Subject)), labels.subject},
    };
}

std::optional<int64_t> toMs(const std::optional<double>& seconds) {
    if (!seconds) return std::nullopt;
    return static_cast<int64_t>(std::llround(*seconds * 1000.0));
}

}  // namespace

InterviewService::InterviewService(IInterviewRepository& repository,
                                    hermes::jobs::IInterviewJobRepository& jobRepository,
                                    hermes::jobs::IJobQueue& jobQueue,
                                    hermes::transcript::ITranscriptStore& transcriptStore)
    : m_repository(repository), m_jobRepository(jobRepository), m_jobQueue(jobQueue), m_transcriptStore(transcriptStore) {}

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
    detail.transcriptEdited = m_transcriptStore.hasEdits(id);
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

TranscriptDocumentOutcome InterviewService::buildDocument(int interviewId, const InterviewRecord& interview) {
    using Status = TranscriptDocumentOutcome::Status;
    const auto transcriptionPath = m_repository.findTranscriptionPathByInterviewId(interviewId);
    if (!transcriptionPath.has_value()) {
        return {Status::NotReady, {}};
    }

    TranscriptDocument document;
    if (auto loaded = m_transcriptStore.loadCurrent(interviewId)) {
        document = TranscriptDocumentBuilder::build(loaded->transcript);
        document.edited = loaded->edited;
    } else {
        // Entrevista procesada antes de la transcripcion estructurada (ADR-022).
        const auto text = readTextFile(*transcriptionPath);
        if (!text.has_value()) {
            log_event("[InterviewService][buildDocument] No se encontro el archivo " + *transcriptionPath +
                      " (interview_id=" + std::to_string(interviewId) + ")");
            return {Status::FileMissing, {}};
        }
        document = TranscriptDocumentBuilder::buildLegacy(*text);
    }

    document.speakers = speakerList(hermes::transcript::makeSpeakerLabels(interview.subjectType));
    if (const auto summaryPath = m_repository.findSummaryPathByInterviewId(interviewId)) {
        document.summary = readTextFile(*summaryPath);
    }
    return {Status::Ok, std::move(document)};
}

TranscriptDocumentOutcome InterviewService::getTranscriptDocument(int interviewId) {
    const auto interview = m_repository.findById(interviewId);
    if (!interview.has_value()) {
        return {TranscriptDocumentOutcome::Status::NotFound, {}};
    }
    return buildDocument(interviewId, *interview);
}

TranscriptTextOutcome InterviewService::renderTranscriptText(int interviewId) {
    using Status = TranscriptTextOutcome::Status;
    const auto interview = m_repository.findById(interviewId);
    if (!interview.has_value()) {
        return {Status::NotFound, ""};
    }
    const auto transcriptionPath = m_repository.findTranscriptionPathByInterviewId(interviewId);
    if (!transcriptionPath.has_value()) {
        return {Status::NotReady, ""};
    }
    if (auto loaded = m_transcriptStore.loadCurrent(interviewId)) {
        return {Status::Ok, hermes::transcript::renderPlainText(loaded->transcript,
                                                               hermes::transcript::makeSpeakerLabels(interview->subjectType))};
    }
    // Entrevista procesada antes de ADR-022: el archivo tal cual.
    auto text = readTextFile(*transcriptionPath);
    if (!text.has_value()) {
        return {Status::FileMissing, ""};
    }
    return {Status::Ok, std::move(*text)};
}

TranscriptEditOutcome InterviewService::updateTranscript(int interviewId, const std::vector<TranscriptBlockInput>& blocks) {
    using Status = TranscriptEditOutcome::Status;
    const std::string idTag = " (interview_id=" + std::to_string(interviewId) + ")";
    const auto interview = m_repository.findById(interviewId);
    if (!interview.has_value()) {
        return {Status::NotFound, "", {}};
    }
    if (interview->status == "processing" || m_jobRepository.hasActiveJob(interviewId)) {
        return {Status::Busy, "", {}};
    }
    if (!m_repository.findTranscriptionPathByInterviewId(interviewId).has_value()) {
        return {Status::NotReady, "", {}};
    }
    const auto current = m_transcriptStore.loadCurrent(interviewId);
    if (!current.has_value()) {
        return {Status::NotEditable, "", {}};
    }

    if (blocks.size() > MAX_EDIT_BLOCKS) {
        return {Status::Invalid, "La transcripcion admite hasta " + std::to_string(MAX_EDIT_BLOCKS) + " bloques", {}};
    }

    StructuredTranscript edited;
    edited.speakerSource = SpeakerSource::Manual;
    // El aviso de no anonimizacion describe el contenido, no la edicion: se
    // conserva para que quien descargue siga sabiendo que hay datos personales.
    edited.notice = current->transcript.notice;
    for (size_t i = 0; i < blocks.size(); ++i) {
        const auto& block = blocks[i];
        const std::string position = "El bloque " + std::to_string(i + 1);
        const std::string text = trim(block.text);
        if (text.empty()) continue;  // bloque vaciado en el editor: se descarta
        if (text.size() > MAX_EDIT_BLOCK_CHARS) {
            return {Status::Invalid, position + " supera los " + std::to_string(MAX_EDIT_BLOCK_CHARS) + " caracteres", {}};
        }
        const bool hasControlChar = std::any_of(text.begin(), text.end(), [](unsigned char c) {
            return std::iscntrl(c) && c != '\n' && c != '\t';
        });
        if (hasControlChar) {
            return {Status::Invalid, position + " contiene caracteres no permitidos", {}};
        }

        TranscriptTurn turn;
        if (block.speaker && !block.speaker->empty()) {
            turn.speaker = hermes::transcript::parseSpeakerRole(*block.speaker);
            if (!turn.speaker) {
                return {Status::Invalid, position + " tiene un hablante desconocido: '" + *block.speaker + "'", {}};
            }
        }
        if ((block.startSeconds && *block.startSeconds < 0) || (block.endSeconds && *block.endSeconds < 0) ||
            (block.startSeconds && block.endSeconds && *block.startSeconds > *block.endSeconds)) {
            return {Status::Invalid, position + " tiene tiempos invalidos", {}};
        }
        turn.startMs = toMs(block.startSeconds);
        turn.endMs = toMs(block.endSeconds);
        turn.text = text;
        edited.turns.push_back(std::move(turn));
    }
    if (edited.turns.empty()) {
        return {Status::Invalid, "La transcripcion editada no puede quedar vacia", {}};
    }

    try {
        m_transcriptStore.saveEdited(interviewId, edited);
    } catch (const std::exception& e) {
        log_event(std::string("[InterviewService][updateTranscript] No se pudo guardar la edicion") + idTag + ": " + e.what());
        return {Status::Failed, "", {}};
    }
    log_event("[InterviewService][updateTranscript] Transcripcion editada guardada (" + std::to_string(edited.turns.size()) +
              " bloques)" + idTag);

    auto document = buildDocument(interviewId, *interview);
    if (document.status != TranscriptDocumentOutcome::Status::Ok) {
        return {Status::Failed, "", {}};
    }
    return {Status::Ok, "", std::move(document.document)};
}

TranscriptEditOutcome InterviewService::restoreTranscript(int interviewId) {
    using Status = TranscriptEditOutcome::Status;
    const auto interview = m_repository.findById(interviewId);
    if (!interview.has_value()) {
        return {Status::NotFound, "", {}};
    }
    if (interview->status == "processing" || m_jobRepository.hasActiveJob(interviewId)) {
        return {Status::Busy, "", {}};
    }
    if (!m_transcriptStore.removeEdited(interviewId)) {
        return {Status::Failed, "", {}};
    }
    auto document = buildDocument(interviewId, *interview);
    switch (document.status) {
        case TranscriptDocumentOutcome::Status::Ok:
            return {Status::Ok, "", std::move(document.document)};
        case TranscriptDocumentOutcome::Status::NotReady:
            return {Status::NotReady, "", {}};
        default:
            return {Status::Failed, "", {}};
    }
}

AudioOutcome InterviewService::readAudio(int interviewId, const std::optional<AudioByteRange>& range) {
    AudioOutcome outcome;
    if (!m_repository.existsById(interviewId)) {
        outcome.status = AudioOutcome::Status::NotFound;
        return outcome;
    }

    // El WAV normalizado (no el upload): es exactamente el audio contra el que
    // se calcularon los tiempos, y el original puede no existir (un video se
    // reemplaza por su audio, ADR-016). Existe desde que se proceso una vez.
    const std::filesystem::path path = std::filesystem::path(std::string(Config::STORAGE_DIRECTORY)) / "interviews" /
                                       std::to_string(interviewId) / "audio.wav";
    std::error_code ec;
    const auto size = std::filesystem::file_size(path, ec);
    if (ec || size == 0) {
        outcome.status = AudioOutcome::Status::NotAvailable;
        return outcome;
    }
    outcome.path = path.string();
    outcome.totalSize = static_cast<long long>(size);

    if (!range) {
        outcome.status = AudioOutcome::Status::Ok;
        return outcome;
    }

    long long start;
    long long end;
    if (!range->start) {
        // Sufijo: los ultimos N bytes.
        const long long suffix = range->end.value_or(0);
        if (suffix <= 0) {
            outcome.status = AudioOutcome::Status::RangeNotSatisfiable;
            return outcome;
        }
        start = std::max(0LL, outcome.totalSize - suffix);
        end = outcome.totalSize - 1;
    } else {
        start = *range->start;
        end = range->end.value_or(outcome.totalSize - 1);
    }
    if (start < 0 || start >= outcome.totalSize || end < start) {
        outcome.status = AudioOutcome::Status::RangeNotSatisfiable;
        return outcome;
    }
    end = std::min({end, outcome.totalSize - 1, start + Config::MAX_AUDIO_RANGE_BYTES - 1});

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        outcome.status = AudioOutcome::Status::NotAvailable;
        return outcome;
    }
    outcome.bytes.resize(static_cast<size_t>(end - start + 1));
    file.seekg(start);
    file.read(outcome.bytes.data(), static_cast<std::streamsize>(outcome.bytes.size()));
    outcome.bytes.resize(static_cast<size_t>(file.gcount()));
    if (outcome.bytes.empty()) {
        outcome.status = AudioOutcome::Status::NotAvailable;
        return outcome;
    }
    outcome.start = start;
    outcome.end = start + static_cast<long long>(outcome.bytes.size()) - 1;
    outcome.partial = true;
    outcome.status = AudioOutcome::Status::Ok;
    return outcome;
}

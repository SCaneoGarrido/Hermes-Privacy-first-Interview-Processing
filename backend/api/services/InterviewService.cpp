#include "../include/services/InterviewService.h"
#include "../include/logger.h"

#include <filesystem>
#include <system_error>

InterviewService::InterviewService(IInterviewRepository& repository) : m_repository(repository) {}

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
    return detail;
}

ProcessOutcome InterviewService::requestProcessing(int id) {
    if (!m_repository.existsById(id)) {
        return ProcessOutcome::NotFound;
    }

    if (!m_repository.findAudioByInterviewId(id).has_value()) {
        return ProcessOutcome::AudioRequired;
    }

    if (!m_repository.updateStatus(id, "processing")) {
        log_event("[InterviewService][requestProcessing] Fallo actualizando el estado a processing, id=" + std::to_string(id));
        return ProcessOutcome::Failed;
    }

    return ProcessOutcome::Ok;
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

    return RemoveOutcome::Ok;
}

bool InterviewService::attachAudio(int interviewId, const std::string& path, const std::string& format, long long size) {
    if (!m_repository.insertAudio(interviewId, path, format, size)) {
        log_event("[InterviewService][attachAudio] Fallo al registrar el audio en la BD");
        return false;
    }

    // No aborta si esto falla (el audio ya quedo guardado y registrado);
    // solo se loguea, igual que el comportamiento previo en FileController.
    if (!m_repository.updateStatus(interviewId, "pending_processing")) {
        log_event("[InterviewService][attachAudio] Fallo actualizando el status de la entrevista a pending_processing");
    }

    return true;
}

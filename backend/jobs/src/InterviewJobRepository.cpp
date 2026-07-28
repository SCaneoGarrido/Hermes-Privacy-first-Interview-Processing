#include "../include/InterviewJobRepository.h"
#include "../../api/include/logger.h"

#include <vector>

namespace hermes::jobs {

InterviewJobRepository::InterviewJobRepository(DatabaseManager& db) : m_db(db) {}

bool InterviewJobRepository::hasActiveJob(int interviewId) {
    std::vector<SqlParam> params = {interviewId};
    auto rows = m_db.executeQuery(
        "SELECT id FROM interview_jobs WHERE interview_id = ? AND status IN ('pending', 'running') LIMIT 1",
        params);
    return !rows.empty();
}

std::optional<int> InterviewJobRepository::createPending(int interviewId) {
    std::string query =
        "INSERT INTO interview_jobs (interview_id, status, created_at, updated_at) "
        "VALUES (?, 'pending', NOW(), NOW())";
    std::vector<SqlParam> params = {interviewId};

    auto result = m_db.executePrepared(query, params, true);
    if (result.has_value() && result.value() > 0) {
        return static_cast<int>(result.value());
    }
    return std::nullopt;
}

bool InterviewJobRepository::markRunning(int interviewId) {
    auto jobId = findJobId(interviewId, "pending");
    if (!jobId.has_value()) {
        log_event("[InterviewJobRepository][markRunning] No hay job pending para interview_id=" + std::to_string(interviewId));
        return false;
    }

    std::vector<SqlParam> params = {jobId.value()};
    auto result = m_db.executePrepared(
        "UPDATE interview_jobs SET status = 'running', started_at = NOW() WHERE id = ?",
        params);
    return result.has_value();
}

bool InterviewJobRepository::markCompleted(int interviewId) {
    auto jobId = findJobId(interviewId, "running");
    if (!jobId.has_value()) {
        log_event("[InterviewJobRepository][markCompleted] No hay job running para interview_id=" + std::to_string(interviewId));
        return false;
    }

    std::vector<SqlParam> params = {jobId.value()};
    auto result = m_db.executePrepared(
        "UPDATE interview_jobs SET status = 'completed', finished_at = NOW() WHERE id = ?",
        params);
    if (!result.has_value()) {
        return false;
    }

    return updateInterviewStatus(interviewId, "completed");
}

bool InterviewJobRepository::markFailed(int interviewId, const std::string& errorMessage) {
    auto jobId = findJobId(interviewId, "running");
    if (!jobId.has_value()) {
        // reclaimStuckJobs pasa por aca con jobs que quedaron en 'running'
        // tras un restart: siempre hay uno. Si no se encuentra nada aca es
        // un estado inesperado, se loguea igual.
        log_event("[InterviewJobRepository][markFailed] No hay job running para interview_id=" + std::to_string(interviewId));
        return false;
    }

    std::vector<SqlParam> params = {errorMessage, jobId.value()};
    auto result = m_db.executePrepared(
        "UPDATE interview_jobs SET status = 'failed', error_message = ?, finished_at = NOW() WHERE id = ?",
        params);
    if (!result.has_value()) {
        return false;
    }

    return updateInterviewStatus(interviewId, "failed");
}

int InterviewJobRepository::reclaimStuckJobs() {
    // 'running': el proceso murio a mitad de un whisper/ollama.
    // 'pending': estaba en JobQueue (in-memory) esperando su turno cuando el
    // proceso murio - la cola no persiste, asi que ese trabajo tambien se
    // perdio aunque nunca haya llegado a running. Sin este segundo caso, un
    // job pending huerfano deja hasActiveJob() en true para siempre: la
    // entrevista queda bloqueada (409 JOB_ALREADY_QUEUED en cualquier
    // reintento) sin que exista ya ningun worker que vaya a procesarlo.
    // No se pasa por markFailed(interviewId, ...) aca porque esa funcion
    // busca especificamente un job 'running' (ver findJobId) y no
    // encontraria las filas 'pending'; se actualiza directo por job id.
    auto rows = m_db.executeQuery("SELECT id, interview_id FROM interview_jobs WHERE status IN ('pending', 'running')");

    int reclaimed = 0;
    for (const auto& row : rows) {
        if (row.size() != 2) {
            continue;
        }
        int jobId = std::get<int>(row[0]);
        int interviewId = std::get<int>(row[1]);

        std::vector<SqlParam> params = {std::string("interrupted by restart"), jobId};
        auto result = m_db.executePrepared(
            "UPDATE interview_jobs SET status = 'failed', error_message = ?, finished_at = NOW() WHERE id = ?",
            params);
        if (!result.has_value()) {
            continue;
        }

        if (updateInterviewStatus(interviewId, "failed")) {
            ++reclaimed;
        }
    }

    if (reclaimed > 0) {
        log_event("[InterviewJobRepository][reclaimStuckJobs] " + std::to_string(reclaimed) + " job(s) en estado pending/running reclamados como failed tras el restart");
    }

    return reclaimed;
}

std::optional<int> InterviewJobRepository::findJobId(int interviewId, const std::string& statusFilter) {
    std::vector<SqlParam> params = {interviewId, statusFilter};
    auto rows = m_db.executeQuery(
        "SELECT id FROM interview_jobs WHERE interview_id = ? AND status = ? ORDER BY id DESC LIMIT 1",
        params);
    if (rows.empty() || rows[0].empty()) {
        return std::nullopt;
    }
    return std::get<int>(rows[0][0]);
}

bool InterviewJobRepository::updateInterviewStatus(int interviewId, const std::string& status) {
    std::vector<SqlParam> params = {status, interviewId};
    auto result = m_db.executePrepared("UPDATE interviews SET status = ? WHERE id = ?", params);
    if (!result.has_value()) {
        log_event("[InterviewJobRepository][updateInterviewStatus] Fallo actualizando interviews.status, interview_id=" + std::to_string(interviewId));
        return false;
    }
    return true;
}

}  // namespace hermes::jobs

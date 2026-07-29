#pragma once

#include "IInterviewJobRepository.h"
#include "../../api/include/DatabaseManager.h"

namespace hermes::jobs {

// Unica clase que sabe hacer INSERT/UPDATE sobre interview_jobs. Ademas de
// la fila de interview_jobs, markCompleted/markFailed tambien actualizan
// interviews.status (el estado "grueso" que consume el frontend).
class InterviewJobRepository : public IInterviewJobRepository {
    public:
        explicit InterviewJobRepository(DatabaseManager& db);

        bool hasActiveJob(int interviewId) override;
        std::optional<int> createPending(int interviewId) override;
        bool markRunning(int interviewId) override;
        bool saveRawTranscriptPath(int interviewId, const std::string& path) override;
        bool updateCurrentStep(int interviewId, const std::string& step) override;
        std::optional<std::string> findCurrentStep(int interviewId) override;
        bool markCompleted(int interviewId) override;
        bool markFailed(int interviewId, const std::string& errorMessage) override;
        int reclaimStuckJobs() override;
        std::optional<long long> findLatestExecutionTimeSeconds(int interviewId) override;

    private:
        DatabaseManager& m_db;

        // id de fila de interview_jobs con status = statusFilter para esa
        // entrevista (la mas reciente). Usado para resolver "el job activo"
        // antes de actualizarlo por id.
        std::optional<int> findJobId(int interviewId, const std::string& statusFilter);
        bool updateInterviewStatus(int interviewId, const std::string& status);
};

}  // namespace hermes::jobs

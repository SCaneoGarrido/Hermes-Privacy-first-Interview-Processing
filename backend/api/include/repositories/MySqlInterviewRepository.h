#ifndef MYSQL_INTERVIEW_REPOSITORY_H
#define MYSQL_INTERVIEW_REPOSITORY_H

#include "IInterviewRepository.h"
#include "../DatabaseManager.h"

// Implementacion de IInterviewRepository sobre DatabaseManager (libmariadb).
class MySqlInterviewRepository : public IInterviewRepository {
    public:
        explicit MySqlInterviewRepository(DatabaseManager& db);

        std::optional<int> create(const std::string& date, const std::string& type, const std::string& subjectType) override;
        std::vector<InterviewRecord> findAll() override;
        std::optional<InterviewRecord> findById(int id) override;
        bool existsById(int id) override;
        std::optional<InterviewAudioRecord> findAudioByInterviewId(int id) override;
        std::optional<std::string> findTranscriptionPathByInterviewId(int id) override;
        bool updateStatus(int id, const std::string& status) override;
        bool insertAudio(int interviewId, const std::string& path, const std::string& format, long long size) override;
        bool remove(int id) override;

    private:
        DatabaseManager& m_db;
};

#endif // MYSQL_INTERVIEW_REPOSITORY_H

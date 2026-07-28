#include "../../include/repositories/MySqlInterviewRepository.h"
#include "../../include/logger.h"

#include <vector>

MySqlInterviewRepository::MySqlInterviewRepository(DatabaseManager& db) : m_db(db) {}

std::optional<int> MySqlInterviewRepository::create(const std::string& date, const std::string& type, const std::string& subjectType) {
    std::string query =
        "INSERT INTO interviews (interview_date, interview_type, interview_subject_type, created_at, updated_at) "
        "VALUES (?, ?, ?, NOW(), NOW())";
    std::vector<SqlParam> params = {date, type, subjectType};

    auto result = m_db.executePrepared(query, params, true);
    if (result.has_value() && result.value() > 0) {
        return static_cast<int>(result.value());
    }
    return std::nullopt;
}

std::vector<InterviewRecord> MySqlInterviewRepository::findAll() {
    std::string query =
        "SELECT id, interview_date, interview_type, interview_subject_type, status, created_at "
        "FROM interviews ORDER BY id DESC";

    auto rows = m_db.executeQuery(query);

    std::vector<InterviewRecord> interviews;
    interviews.reserve(rows.size());

    for (const auto& row : rows) {
        if (row.size() != 6) {
            log_event("[MySqlInterviewRepository][findAll] Fila con cantidad de columnas inesperada, se omite");
            continue;
        }

        interviews.push_back(InterviewRecord{
            std::get<int>(row[0]),
            std::get<std::string>(row[1]),
            std::get<std::string>(row[2]),
            std::get<std::string>(row[3]),
            std::get<std::string>(row[4]),
            std::get<std::string>(row[5])
        });
    }

    return interviews;
}

std::optional<InterviewRecord> MySqlInterviewRepository::findById(int id) {
    std::vector<SqlParam> idParam = {id};

    auto rows = m_db.executeQuery(
        "SELECT id, interview_date, interview_type, interview_subject_type, status, created_at "
        "FROM interviews WHERE id = ?",
        idParam);

    if (rows.empty()) {
        return std::nullopt;
    }

    const auto& row = rows[0];
    if (row.size() != 6) {
        log_event("[MySqlInterviewRepository][findById] Fila de entrevista con cantidad de columnas inesperada");
        return std::nullopt;
    }

    return InterviewRecord{
        std::get<int>(row[0]),
        std::get<std::string>(row[1]),
        std::get<std::string>(row[2]),
        std::get<std::string>(row[3]),
        std::get<std::string>(row[4]),
        std::get<std::string>(row[5])
    };
}

bool MySqlInterviewRepository::existsById(int id) {
    std::vector<SqlParam> idParam = {id};
    auto rows = m_db.executeQuery("SELECT id FROM interviews WHERE id = ?", idParam);
    return !rows.empty();
}

std::optional<InterviewAudioRecord> MySqlInterviewRepository::findAudioByInterviewId(int id) {
    std::vector<SqlParam> idParam = {id};

    auto rows = m_db.executeQuery(
        "SELECT interview_audio_path, interview_audio_format, interview_audio_size "
        "FROM interviews_audio WHERE interview_id = ?",
        idParam);

    if (rows.empty() || rows[0].size() != 3) {
        return std::nullopt;
    }

    return InterviewAudioRecord{
        std::get<std::string>(rows[0][0]),
        std::get<std::string>(rows[0][1]),
        std::get<long long>(rows[0][2])
    };
}

std::optional<std::string> MySqlInterviewRepository::findTranscriptionPathByInterviewId(int id) {
    std::vector<SqlParam> idParam = {id};

    auto rows = m_db.executeQuery(
        "SELECT interview_transcription_file_path FROM interview_results WHERE interview_id = ?",
        idParam);

    if (rows.empty() || rows[0].size() != 1) {
        return std::nullopt;
    }

    return std::get<std::string>(rows[0][0]);
}

bool MySqlInterviewRepository::updateStatus(int id, const std::string& status) {
    std::vector<SqlParam> params = {status, id};
    auto result = m_db.executePrepared("UPDATE interviews SET status = ? WHERE id = ?", params);
    return result.has_value();
}

bool MySqlInterviewRepository::insertAudio(int interviewId, const std::string& path, const std::string& format, long long size) {
    std::string query =
        "INSERT INTO interviews_audio (interview_audio_path, interview_audio_format, interview_audio_size, interview_id, created_at) "
        "VALUES (?, ?, ?, ?, NOW())";
    std::vector<SqlParam> params = {path, format, size, interviewId};

    auto result = m_db.executePrepared(query, params);
    return result.has_value();
}

bool MySqlInterviewRepository::upsertTranscriptionResult(int interviewId, const std::string& path) {
    std::string query =
        "INSERT INTO interview_results (interview_transcription_file_path, interview_id, created_at) "
        "VALUES (?, ?, NOW()) "
        "ON DUPLICATE KEY UPDATE interview_transcription_file_path = VALUES(interview_transcription_file_path)";
    std::vector<SqlParam> params = {path, interviewId};

    auto result = m_db.executePrepared(query, params);
    return result.has_value();
}

bool MySqlInterviewRepository::remove(int id) {
    std::vector<SqlParam> idParam = {id};
    auto result = m_db.executePrepared("DELETE FROM interviews WHERE id = ?", idParam);
    return result.has_value();
}

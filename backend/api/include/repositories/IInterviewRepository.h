#ifndef I_INTERVIEW_REPOSITORY_H
#define I_INTERVIEW_REPOSITORY_H

#include <optional>
#include <string>
#include <vector>

// Fila de la tabla interviews.
struct InterviewRecord {
    int id;
    std::string date;
    std::string type;
    std::string subjectType;
    std::string status;
    std::string createdAt;
};

// Fila de la tabla interviews_audio (a lo sumo una por entrevista, ver
// UNIQUE(interview_id) en SQL/init.sql).
struct InterviewAudioRecord {
    std::string path;
    std::string format;
    long long size;
};

// Oculta MySQL/DatabaseManager detras de esta interfaz (ver Filosofia de
// Repositorios en .ai/PROJECT.md). InterviewController y FileController
// dependen unicamente de esto, nunca de DatabaseManager.
class IInterviewRepository {
    public:
        virtual ~IInterviewRepository() = default;

        virtual std::optional<int> create(const std::string& date, const std::string& type, const std::string& subjectType) = 0;
        virtual std::vector<InterviewRecord> findAll() = 0;
        virtual std::optional<InterviewRecord> findById(int id) = 0;
        // Chequeo de existencia liviano (SELECT id), mas barato que findById
        // cuando no hace falta el resto de las columnas.
        virtual bool existsById(int id) = 0;
        virtual std::optional<InterviewAudioRecord> findAudioByInterviewId(int id) = 0;
        virtual std::optional<std::string> findTranscriptionPathByInterviewId(int id) = 0;
        virtual bool updateStatus(int id, const std::string& status) = 0;
        virtual bool insertAudio(int interviewId, const std::string& path, const std::string& format, long long size) = 0;
        // Guarda/reemplaza el resultado final (Sprint 5/6). UPSERT: interview_id
        // es UNIQUE en interview_results, y una entrevista puede reprocesarse
        // (ver Sprint 4) despues de ya haber tenido un resultado previo.
        virtual bool upsertTranscriptionResult(int interviewId, const std::string& path) = 0;
        // Borra la entrevista. interviews_audio / interview_results se
        // eliminan solos via ON DELETE CASCADE (SQL/init.sql).
        virtual bool remove(int id) = 0;
};

#endif // I_INTERVIEW_REPOSITORY_H

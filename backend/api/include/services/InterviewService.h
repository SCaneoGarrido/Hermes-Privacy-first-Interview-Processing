#ifndef INTERVIEW_SERVICE_H
#define INTERVIEW_SERVICE_H

#include "../repositories/IInterviewRepository.h"
#include <optional>
#include <string>
#include <vector>

// Entrevista + hijas opcionales, tal como las necesita GET /interview/:id.
struct InterviewDetailRecord {
    InterviewRecord interview;
    std::optional<InterviewAudioRecord> audio;
    std::optional<std::string> transcriptionPath;
};

enum class ProcessOutcome { NotFound, AudioRequired, Ok, Failed };
enum class RemoveOutcome { NotFound, Ok, Failed };

// Orquesta las reglas de negocio de entrevistas sobre IInterviewRepository.
// No depende de Crow ni de MySQL (ver Core Principles, .ai/PROJECT.md):
// los controllers traducen estos resultados a HTTP.
class InterviewService {
    public:
        explicit InterviewService(IInterviewRepository& repository);

        std::optional<int> createInterview(const std::string& date, const std::string& type, const std::string& subjectType);
        std::vector<InterviewRecord> listInterviews();
        std::optional<InterviewDetailRecord> getInterviewDetail(int id);
        ProcessOutcome requestProcessing(int id);
        RemoveOutcome removeInterview(int id);
        // Usado por FileController al recibir un audio: inserta el audio y
        // marca la entrevista como pending_processing. El fallo al
        // actualizar el status no es fatal (el audio ya quedo guardado en
        // disco y registrado en la BD); solo se loguea.
        bool attachAudio(int interviewId, const std::string& path, const std::string& format, long long size);

    private:
        IInterviewRepository& m_repository;
};

#endif // INTERVIEW_SERVICE_H

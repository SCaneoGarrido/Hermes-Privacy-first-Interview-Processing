#ifndef INTERVIEW_SERVICE_H
#define INTERVIEW_SERVICE_H

#include "../repositories/IInterviewRepository.h"
#include "../../../jobs/include/IInterviewJobRepository.h"
#include "../../../jobs/include/IJobQueue.h"
#include <optional>
#include <string>
#include <vector>

// Entrevista + hijas opcionales, tal como las necesita GET /interview/:id.
struct InterviewDetailRecord {
    InterviewRecord interview;
    std::optional<InterviewAudioRecord> audio;
    std::optional<std::string> transcriptionPath;
};

// AlreadyQueued: ya existe un job pending/running para esta entrevista
// (Sprint 4 - Background Processing); evita doble-encolado.
enum class ProcessOutcome { NotFound, AudioRequired, AlreadyQueued, Ok, Failed };
enum class RemoveOutcome { NotFound, Ok, Failed };

// Orquesta las reglas de negocio de entrevistas sobre IInterviewRepository.
// No depende de Crow ni de MySQL (ver Core Principles, .ai/PROJECT.md):
// los controllers traducen estos resultados a HTTP. hermes::jobs::IInterviewJobRepository
// y hermes::jobs::IJobQueue son abstracciones igual de finas (Filosofia de
// Repositorios) - no meten a DatabaseManager en este archivo.
class InterviewService {
    public:
        InterviewService(IInterviewRepository& repository,
                          hermes::jobs::IInterviewJobRepository& jobRepository,
                          hermes::jobs::IJobQueue& jobQueue);

        std::optional<int> createInterview(const std::string& date, const std::string& type, const std::string& subjectType);
        std::vector<InterviewRecord> listInterviews();
        std::optional<InterviewDetailRecord> getInterviewDetail(int id);
        // Valida existencia + audio + que no haya ya un job en curso: si
        // pasa, encola un job de procesamiento (Sprint 4) ademas de marcar
        // la entrevista como processing.
        ProcessOutcome requestProcessing(int id);
        RemoveOutcome removeInterview(int id);
        // Usado por FileController al recibir un audio: inserta el audio y
        // marca la entrevista como pending_processing. El fallo al
        // actualizar el status no es fatal (el audio ya quedo guardado en
        // disco y registrado en la BD); solo se loguea.
        bool attachAudio(int interviewId, const std::string& path, const std::string& format, long long size);

    private:
        IInterviewRepository& m_repository;
        hermes::jobs::IInterviewJobRepository& m_jobRepository;
        hermes::jobs::IJobQueue& m_jobQueue;
};

#endif // INTERVIEW_SERVICE_H

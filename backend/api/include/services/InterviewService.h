#ifndef INTERVIEW_SERVICE_H
#define INTERVIEW_SERVICE_H

#include "../repositories/IInterviewRepository.h"
#include "TranscriptDocument.h"
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
    std::optional<std::string> summaryPath;
    // Segundos entre started_at/finished_at del job mas reciente con ambos
    // timestamps seteados (Sprint 6) - calculado, no una columna nueva
    // (ver whisper.cpp Architecture, "Execution Time").
    std::optional<long long> executionTimeSeconds;
    // Paso grueso actual mientras hay un job 'running' (ver Job.h /
    // IInterviewJobRepository::updateCurrentStep) - feedback de progreso
    // para que la UI no parezca trabada en procesamientos largos.
    std::optional<std::string> currentStep;
    // Glosario de palabras clave (ADR-018); vacio si no se cargo.
    std::vector<std::string> keywords;
};

// AlreadyQueued: ya existe un job pending/running para esta entrevista
// (Sprint 4 - Background Processing); evita doble-encolado.
enum class ProcessOutcome { NotFound, AudioRequired, AlreadyQueued, Ok, Failed };
enum class RemoveOutcome { NotFound, Ok, Failed };
// Busy: la entrevista se esta procesando (o tiene un job encolado); su audio
// no puede cambiarse hasta que termine.
enum class AttachAudioOutcome { NotFound, Busy, Ok, Failed };

// NotReady: la entrevista todavia no tiene resultado. FileMissing: la BD
// registra un resultado pero el archivo ya no esta en disco.
struct TranscriptDocumentOutcome {
    enum class Status { NotFound, NotReady, FileMissing, Ok } status;
    TranscriptDocument document;
};

// Resultado de setKeywords: en Ok, keywords es la lista ya normalizada; en
// Invalid, message explica que regla no se cumplio.
struct KeywordsOutcome {
    enum class Status { NotFound, Invalid, Ok, Failed } status;
    std::string message;
    std::vector<std::string> keywords;
};

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
        // la entrevista como processing. includeSummary / enhanceTranscript:
        // fases opcionales via Ollama (ver Job.h), ambas default false - la
        // salida por defecto es solo la transcripcion de whisper.
        ProcessOutcome requestProcessing(int id, bool includeSummary = false, bool enhanceTranscript = false);
        RemoveOutcome removeInterview(int id);
        // Reemplaza el glosario de la entrevista. Normaliza (trim, descarta
        // vacios, deduplica sin distinguir mayusculas) y valida los limites
        // de Config::MAX_KEYWORDS / MAX_KEYWORD_LENGTH. Lista vacia: lo borra.
        KeywordsOutcome setKeywords(int id, const std::vector<std::string>& keywords);
        // Chequeo previo a guardar el archivo en disco: evita escribir hasta
        // 1 GB para una entrevista que no existe o que se esta procesando.
        AttachAudioOutcome canAttachAudio(int interviewId);
        // Asocia el archivo ya guardado en `path` a la entrevista y la marca
        // como pending_processing. Si ya tenia audio lo reemplaza: borra el
        // archivo anterior y el resultado previo (transcripcion/resumen en
        // disco y su fila), que ya no corresponden al audio nuevo. Si no
        // devuelve Ok, borra `path`: un upload rechazado nunca queda en disco.
        AttachAudioOutcome attachAudio(int interviewId, const std::string& path, const std::string& format, long long size);
        // Transcripcion final estructurada para la vista de lectura (ver
        // TranscriptDocumentBuilder), con el resumen si existe.
        TranscriptDocumentOutcome getTranscriptDocument(int interviewId);

    private:
        // Borra storage/interviews/<id> (transcripciones, resumen, audio
        // normalizado). Best-effort: si falla solo se loguea.
        void removeProcessingOutputs(int interviewId);

        IInterviewRepository& m_repository;
        hermes::jobs::IInterviewJobRepository& m_jobRepository;
        hermes::jobs::IJobQueue& m_jobQueue;
};

#endif // INTERVIEW_SERVICE_H

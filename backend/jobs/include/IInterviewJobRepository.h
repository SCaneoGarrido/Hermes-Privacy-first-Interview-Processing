#pragma once

#include <optional>
#include <string>

namespace hermes::jobs {

// Oculta MySQL/DatabaseManager detras de esta interfaz, igual que
// IInterviewRepository (ver ADR-012). Los workers no tocan DatabaseManager
// directamente, solo hablan con esto.
//
// markRunning/markCompleted/markFailed operan por interviewId (no por
// job id): Job, lo que viaja por la cola en memoria, solo lleva el
// interview_id (ver Job.h). Como hasActiveJob impide que exista mas de un
// job pending/running a la vez para una misma entrevista, "el job activo
// de esta entrevista" identifica una fila sin ambiguedad.
class IInterviewJobRepository {
    public:
        virtual ~IInterviewJobRepository() = default;

        // true si ya existe una fila pending o running para esa entrevista -
        // usado por InterviewService.requestProcessing para evitar
        // doble-encolado (409 en el controller).
        virtual bool hasActiveJob(int interviewId) = 0;

        virtual std::optional<int> createPending(int interviewId) = 0;

        virtual bool markRunning(int interviewId) = 0;
        // Tambien actualiza interviews.status a 'completed'.
        virtual bool markCompleted(int interviewId) = 0;
        // Tambien actualiza interviews.status a 'failed'.
        virtual bool markFailed(int interviewId, const std::string& errorMessage) = 0;

        // Al arrancar (main.cpp, junto a migrateTables): cualquier job que
        // haya quedado en 'running' murio a mitad de un whisper/ollama
        // porque el proceso se reinicio; cualquiera en 'pending' estaba en
        // JobQueue (in-memory, no persiste) esperando turno y se perdio con
        // el proceso. Ambos se marcan failed para que la entrevista se
        // pueda reprocesar en vez de quedar bloqueada para siempre.
        // Devuelve la cantidad de jobs reclamados.
        virtual int reclaimStuckJobs() = 0;
};

}  // namespace hermes::jobs

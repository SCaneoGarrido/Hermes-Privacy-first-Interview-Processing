#pragma once

#include "Job.h"

#include <optional>

namespace hermes::jobs {

// Cola de trabajos thread-safe. enqueue() la llama el thread de Crow que
// atendio el POST /interview/:id/process - tiene que ser rapido (solo
// push + notify) para no bloquear el request. dequeue() la llaman los
// threads de WorkerPool y bloquean hasta que haya trabajo o se pida stop().
class IJobQueue {
    public:
        virtual ~IJobQueue() = default;

        virtual void enqueue(Job job) = 0;

        // Bloquea hasta que haya un job disponible. Devuelve nullopt
        // unicamente cuando stop() fue invocado y no queda trabajo
        // pendiente en la cola - es la señal para que el worker termine.
        virtual std::optional<Job> dequeue() = 0;

        // Despierta a todos los threads bloqueados en dequeue() para que
        // puedan salir de su loop. Idempotente.
        virtual void stop() = 0;
};

}  // namespace hermes::jobs

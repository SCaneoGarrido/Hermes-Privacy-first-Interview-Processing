#pragma once

#include "IJobHandler.h"
#include "IInterviewJobRepository.h"
#include "IJobQueue.h"

#include <thread>
#include <vector>

namespace hermes::jobs {

// N threads (configurable via env var WORKER_POOL_SIZE, default 1: Whisper/
// Ollama son CPU/GPU-bound, no conviene paralelizar de mas en una laptop de
// researcher). Cada thread hace dequeue -> markRunning -> handler.execute
// -> markCompleted/markFailed en loop hasta que la cola se detiene.
//
// RAII estilo DatabaseManager: el destructor detiene la cola y hace join()
// de los threads. Esto protege un shutdown limpio via Ctrl+C (main.cpp
// bloquea en Crow::run(); cuando run() retorna, WorkerPool se destruye al
// salir de scope) - no protege contra kill -9, aceptable para desarrollo
// local.
class WorkerPool {
    public:
        WorkerPool(IJobQueue& queue, IJobHandler& handler, IInterviewJobRepository& repository, std::size_t poolSize);
        ~WorkerPool();

        WorkerPool(const WorkerPool&) = delete;
        WorkerPool& operator=(const WorkerPool&) = delete;

    private:
        IJobQueue& m_queue;
        IJobHandler& m_handler;
        IInterviewJobRepository& m_repository;
        std::vector<std::thread> m_threads;

        void run();
};

}  // namespace hermes::jobs

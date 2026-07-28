#include "../include/WorkerPool.h"
#include "../../api/include/logger.h"

namespace hermes::jobs {

WorkerPool::WorkerPool(IJobQueue& queue, IJobHandler& handler, IInterviewJobRepository& repository, std::size_t poolSize)
    : m_queue(queue), m_handler(handler), m_repository(repository) {
    m_threads.reserve(poolSize);
    for (std::size_t i = 0; i < poolSize; ++i) {
        m_threads.emplace_back(&WorkerPool::run, this);
    }
}

WorkerPool::~WorkerPool() {
    m_queue.stop();
    for (auto& thread : m_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void WorkerPool::run() {
    while (true) {
        auto job = m_queue.dequeue();
        if (!job.has_value()) {
            // stop() fue invocado y no queda trabajo pendiente.
            return;
        }

        if (!m_repository.markRunning(job->interviewId)) {
            log_event("[WorkerPool][run] No se pudo marcar como running el job de interview_id=" + std::to_string(job->interviewId));
            continue;
        }

        try {
            m_handler.execute(*job);
            if (!m_repository.markCompleted(job->interviewId)) {
                log_event("[WorkerPool][run] No se pudo marcar como completed el job de interview_id=" + std::to_string(job->interviewId));
            }
        } catch (const std::exception& e) {
            log_event("[WorkerPool][run] Fallo procesando interview_id=" + std::to_string(job->interviewId) + ". Detalle: " + e.what());
            if (!m_repository.markFailed(job->interviewId, e.what())) {
                log_event("[WorkerPool][run] No se pudo marcar como failed el job de interview_id=" + std::to_string(job->interviewId));
            }
        } catch (...) {
            log_event("[WorkerPool][run] Fallo procesando interview_id=" + std::to_string(job->interviewId) + " con una excepcion de tipo desconocido");
            if (!m_repository.markFailed(job->interviewId, "unknown error")) {
                log_event("[WorkerPool][run] No se pudo marcar como failed el job de interview_id=" + std::to_string(job->interviewId));
            }
        }
    }
}

}  // namespace hermes::jobs

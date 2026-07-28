#include "../include/JobQueue.h"

namespace hermes::jobs {

void JobQueue::enqueue(Job job) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(std::move(job));
    }
    m_cv.notify_one();
}

std::optional<Job> JobQueue::dequeue() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv.wait(lock, [this]() { return m_stopped || !m_queue.empty(); });

    if (m_queue.empty()) {
        // Solo se llega aca si m_stopped es true (la condicion de arriba lo
        // garantiza) y no quedo trabajo pendiente por drenar.
        return std::nullopt;
    }

    Job job = std::move(m_queue.front());
    m_queue.pop();
    return job;
}

void JobQueue::stop() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stopped = true;
    }
    m_cv.notify_all();
}

}  // namespace hermes::jobs

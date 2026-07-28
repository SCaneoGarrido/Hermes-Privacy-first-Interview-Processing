#pragma once

#include "IJobQueue.h"

#include <condition_variable>
#include <mutex>
#include <queue>

namespace hermes::jobs {

// Implementacion in-memory de IJobQueue: std::queue + mutex + condition_variable.
// Se pierde en un restart (no persiste a disco) - aceptable porque
// reclaimStuckJobs() en el arranque limpia cualquier job que haya quedado
// a medias (ver InterviewJobRepository).
class JobQueue : public IJobQueue {
    public:
        void enqueue(Job job) override;
        std::optional<Job> dequeue() override;
        void stop() override;

    private:
        std::mutex m_mutex;
        std::condition_variable m_cv;
        std::queue<Job> m_queue;
        bool m_stopped = false;
};

}  // namespace hermes::jobs

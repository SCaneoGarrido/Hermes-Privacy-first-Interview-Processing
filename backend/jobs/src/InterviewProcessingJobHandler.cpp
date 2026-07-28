#include "../include/InterviewProcessingJobHandler.h"
#include "../../api/include/logger.h"

#include <chrono>
#include <thread>

namespace hermes::jobs {

void InterviewProcessingJobHandler::execute(const Job& job) {
    log_event("[InterviewProcessingJobHandler][execute] Stub: simulando procesamiento de interview_id=" +
               std::to_string(job.interviewId));

    // Simula trabajo costoso (lo que hoy hariam Whisper/Ollama) sin
    // bloquear el thread de Crow: esto corre en un worker thread.
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

}  // namespace hermes::jobs

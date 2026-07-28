#pragma once

#include <chrono>
#include <string>

namespace hermes::jobs {

// Struct de valor: lo que viaja por la cola en memoria. No es la fila de
// interview_jobs (eso lo modela InterviewJobRecord) - Job es solo lo minimo
// que un worker necesita para saber que ejecutar.
struct Job {
    int interviewId;
    // Unico tipo por ahora ("interview_processing"); existe para no romper
    // el contrato de IJobHandler cuando Sprint 5/6 agreguen mas tipos.
    std::string type = "interview_processing";
    std::chrono::system_clock::time_point createdAt = std::chrono::system_clock::now();
};

}  // namespace hermes::jobs

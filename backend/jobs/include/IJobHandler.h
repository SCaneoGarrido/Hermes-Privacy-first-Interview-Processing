#pragma once

#include "Job.h"

namespace hermes::jobs {

// La pieza que Sprint 5 (Whisper) y Sprint 6 (Ollama) van a implementar o
// extender. WorkerPool no sabe nada de transcripcion/LLM: solo llama
// execute() y traduce el resultado a markCompleted/markFailed.
class IJobHandler {
    public:
        virtual ~IJobHandler() = default;

        // Debe lanzar (std::exception o derivada) si el procesamiento falla;
        // WorkerPool captura la excepcion y usa what() como error_message.
        virtual void execute(const Job& job) = 0;
};

}  // namespace hermes::jobs

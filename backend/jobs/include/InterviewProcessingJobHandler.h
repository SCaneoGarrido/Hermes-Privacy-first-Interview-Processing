#pragma once

#include "IJobHandler.h"

namespace hermes::jobs {

// Stub para Sprint 4: simula el trabajo (Whisper + Ollama todavia no
// existen, ver .ai/PROJECT.md) para que el pipeline completo - encolar,
// correr en un worker thread, marcar completed - sea deployable sin
// depender de Sprint 5/6. Sprint 5 reemplaza/extiende execute() con la
// transcripcion real.
class InterviewProcessingJobHandler : public IJobHandler {
    public:
        void execute(const Job& job) override;
};

}  // namespace hermes::jobs

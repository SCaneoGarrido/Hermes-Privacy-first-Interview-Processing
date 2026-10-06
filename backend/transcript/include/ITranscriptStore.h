#pragma once

#include "TranscriptModel.h"

#include <optional>

namespace hermes::transcript {

struct LoadedTranscript {
    StructuredTranscript transcript;
    // true si es la version editada a mano por el usuario (no la del pipeline).
    bool edited = false;
};

// Persistencia de la transcripcion estructurada de cada entrevista. Dos
// versiones: la que produce el pipeline (nunca la tocan las ediciones, queda
// para trazabilidad) y la editada por el usuario, que tiene prioridad al leer.
class ITranscriptStore {
    public:
        virtual ~ITranscriptStore() = default;

        // Lanzan std::runtime_error si no se puede escribir.
        virtual void savePipeline(int interviewId, const StructuredTranscript& transcript) = 0;
        virtual void saveEdited(int interviewId, const StructuredTranscript& transcript) = 0;

        // Descarta la version editada (si existe). Devuelve false solo si
        // existia y no se pudo borrar.
        virtual bool removeEdited(int interviewId) = 0;

        // La editada si existe, si no la del pipeline; nullopt si no hay
        // ninguna (entrevista sin procesar o procesada antes de este formato).
        virtual std::optional<LoadedTranscript> loadCurrent(int interviewId) = 0;

        virtual bool hasEdits(int interviewId) = 0;
};

}  // namespace hermes::transcript

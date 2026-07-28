#pragma once

#include "ITranscriber.h"

#include <memory>
#include <mutex>
#include <string>

struct whisper_context;

namespace hermes::transcription {

// Implementacion sobre whisper.cpp (vcpkg: whisper-cpp + ggml, linkeados
// estatico). El modelo (~150MB, ver whisper.cpp Architecture en la vault)
// NO se descarga automaticamente ni se carga en el constructor: cargarlo
// es costoso, y si el composition root fallara con el proceso entero
// porque el modelo todavia no esta en disco, un researcher que recien
// clona el repo no podria ni levantar el backend para explorar el resto
// de la API. En cambio, se carga una unica vez, en el primer transcribe(),
// con doble chequeo bajo mutex.
//
// Ese mismo mutex serializa tambien whisper_full(): el contexto no es
// seguro para llamadas concurrentes sobre la misma instancia (ver
// whisper.cpp Architecture, Best Practices). Con WORKER_POOL_SIZE=1
// (default) nunca hay contencion real; si algun dia sube, esto sigue
// siendo correcto en vez de crashear.
class WhisperTranscriber : public ITranscriber {
    public:
        // language: codigo de idioma de whisper.cpp ("es", "en", ...) o
        // "auto" para deteccion automatica. "auto" agrega una pasada extra
        // de deteccion y es menos confiable en clips cortos/ruidosos (ver
        // whisper.cpp Architecture, Common Mistakes) - forzar el idioma
        // cuando se conoce de antemano (la mayoria de entrevistas de Hermes
        // son en espanol) es mas rapido y mas preciso.
        WhisperTranscriber(std::string modelPath, std::string language);
        ~WhisperTranscriber() override;

        WhisperTranscriber(const WhisperTranscriber&) = delete;
        WhisperTranscriber& operator=(const WhisperTranscriber&) = delete;

        std::vector<TranscriptSegment> transcribe(const std::string& audioPath) override;

    private:
        std::string m_modelPath;
        std::string m_language;
        std::mutex m_mutex;
        whisper_context* m_context = nullptr;

        // Debe llamarse con m_mutex ya tomado.
        void ensureModelLoaded();
};

}  // namespace hermes::transcription

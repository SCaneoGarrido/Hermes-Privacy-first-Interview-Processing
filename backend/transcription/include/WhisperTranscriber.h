#pragma once

#include "ITranscriber.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

struct whisper_context;
struct whisper_full_params;

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
// Segmento interno con tiempos numericos (centisegundos), necesarios para
// re-transcribir un rango; se formatean a "HH:MM:SS" recien al final.
struct RawSegment {
    int64_t t0;
    int64_t t1;
    std::string text;
};

class WhisperTranscriber : public ITranscriber {
    public:
        // language: codigo de idioma de whisper.cpp ("es", "en", ...) o
        // "auto" para deteccion automatica. "auto" agrega una pasada extra
        // de deteccion y es menos confiable en clips cortos/ruidosos (ver
        // whisper.cpp Architecture, Common Mistakes) - forzar el idioma
        // cuando se conoce de antemano (la mayoria de entrevistas de Hermes
        // son en espanol) es mas rapido y mas preciso.
        // vadModelPath: modelo Silero de whisper.cpp (ggml-silero-*.bin). Si
        // el archivo no existe se transcribe sin VAD (con aviso en el log),
        // igual que el modelo principal no bloquea el arranque del backend.
        WhisperTranscriber(std::string modelPath, std::string language, std::string vadModelPath);
        ~WhisperTranscriber() override;

        WhisperTranscriber(const WhisperTranscriber&) = delete;
        WhisperTranscriber& operator=(const WhisperTranscriber&) = delete;

        std::vector<TranscriptSegment> transcribe(const std::string& audioPath,
                                                  const TranscriptionOptions& options) override;

    private:
        std::string m_modelPath;
        std::string m_language;
        std::string m_vadModelPath;
        std::mutex m_mutex;
        whisper_context* m_context = nullptr;

        // Debe llamarse con m_mutex ya tomado.
        void ensureModelLoaded();

        // Arma "Glosario: a, b, c." con los primeros terminos que entran en
        // MAX_PROMPT_TOKENS. promptTokens: cuantos tokens ocupa (0 si no hay
        // glosario). Debe llamarse con el modelo ya cargado.
        std::string buildGlossaryPrompt(const std::vector<std::string>& keywords, int& promptTokens);

        // Corre whisper_full sobre [samples, samples + sampleCount) y devuelve
        // los segmentos con tiempos en centisegundos, desplazados offsetCs
        // (para re-transcribir un tramo y ubicarlo en el audio completo).
        // Debe llamarse con m_mutex ya tomado.
        std::vector<RawSegment> runWhisper(whisper_full_params params,
                                           const float* samples,
                                           size_t sampleCount,
                                           int64_t offsetCs,
                                           bool useVad);
};

}  // namespace hermes::transcription

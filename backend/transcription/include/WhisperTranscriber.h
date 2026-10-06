#pragma once

#include "ITranscriber.h"

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>

struct whisper_context;
struct whisper_full_params;

namespace hermes::transcription {

// Auto: usa GPU (Vulkan) si ggml detecta un dispositivo, si no CPU.
// On: intenta GPU; si la carga falla, cae a CPU igual (con aviso).
// Off: siempre CPU (escape si el backend GPU da resultados raros).
enum class GpuMode { Auto, On, Off };

struct WhisperOptions {
    std::string modelPath;
    // Codigo de idioma de whisper.cpp ("es", "en", ...) o "auto". "auto"
    // agrega una pasada extra de deteccion y es menos confiable en clips
    // cortos/ruidosos (ver whisper.cpp Architecture, Common Mistakes) -
    // forzar el idioma cuando se conoce de antemano es mas rapido y preciso.
    std::string language;
    // Modelo Silero de whisper.cpp (ggml-silero-*.bin). Si el archivo no
    // existe se transcribe sin VAD (con aviso en el log).
    std::string vadModelPath;
    GpuMode gpuMode = GpuMode::Auto;
};

// Segmento interno con tiempos numericos en centisegundos (la unidad de
// whisper.cpp), necesarios para re-transcribir un rango; se convierten a
// TranscriptSegment (ms) recien al final.
struct RawSegment {
    int64_t t0;
    int64_t t1;
    std::string text;
    std::vector<TranscriptWord> words;
};

// Implementacion sobre whisper.cpp (vcpkg: whisper-cpp + ggml, linkeados
// estatico; backend Vulkan opcional, ver ADR-020). El modelo NO se descarga
// automaticamente ni se carga en el constructor: cargarlo es costoso, y si
// el composition root fallara porque el modelo todavia no esta en disco,
// un researcher que recien clona el repo no podria ni levantar el backend.
// Se carga una unica vez, en el primer transcribe(), bajo mutex.
//
// Ese mismo mutex serializa tambien whisper_full(): el contexto no es
// seguro para llamadas concurrentes sobre la misma instancia (ver
// whisper.cpp Architecture, Best Practices).
class WhisperTranscriber : public ITranscriber {
    public:
        explicit WhisperTranscriber(WhisperOptions options);
        ~WhisperTranscriber() override;

        WhisperTranscriber(const WhisperTranscriber&) = delete;
        WhisperTranscriber& operator=(const WhisperTranscriber&) = delete;

        std::vector<TranscriptSegment> transcribe(const std::string& audioPath,
                                                  const TranscriptionOptions& options) override;

    private:
        WhisperOptions m_options;
        std::mutex m_mutex;
        whisper_context* m_context = nullptr;
        // true si el modelo quedo cargado sobre GPU: habilita beam search
        // en la pasada principal (en CPU es demasiado lento).
        bool m_gpuActive = false;

        // Debe llamarse con m_mutex ya tomado.
        void ensureModelLoaded();

        // Arma "Glosario: a, b, c. <STYLE_PROMPT>" con los primeros terminos que
        // entran en MAX_PROMPT_TOKENS (sin keywords, solo la frase de estilo).
        // promptTokens: cuantos tokens ocupa. Debe llamarse con el modelo ya cargado.
        std::string buildInitialPrompt(const std::vector<std::string>& keywords, int& promptTokens);

        // Corre whisper_full sobre [samples, samples + sampleCount) y devuelve
        // los segmentos con tiempos en centisegundos, desplazados offsetCs
        // (para re-transcribir un tramo y ubicarlo en el audio completo).
        // Debe llamarse con m_mutex ya tomado.
        std::vector<RawSegment> runWhisper(whisper_full_params params,
                                           const float* samples,
                                           size_t sampleCount,
                                           int64_t offsetCs,
                                           bool useVad);

        // Palabras del segmento i del ultimo whisper_full, con tiempos en ms
        // ubicados dentro de [segT0Cs, segT1Cs] (ya remapeados).
        std::vector<TranscriptWord> extractWords(int segmentIndex, int64_t segT0Cs, int64_t segT1Cs);
};

}  // namespace hermes::transcription

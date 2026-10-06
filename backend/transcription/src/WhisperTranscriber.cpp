#include "../include/WhisperTranscriber.h"
#include "../../api/include/logger.h"
#include "../../audio/include/WavReader.h"

#include <ggml-backend.h>
#include <whisper.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <vector>

namespace hermes::transcription {

namespace {

constexpr int SAMPLE_RATE = 16000;
constexpr int64_t SAMPLES_PER_CENTISECOND = SAMPLE_RATE / 100;

// Tokens de texto previo (de la ventana anterior) con los que se condiciona
// cada ventana de 30s en la pasada principal: ninguno. El texto previo es lo
// que arrastra derivas hasta el final del audio:
// - entrevista 13: "que no se cumplen los plazos" x114 durante 43:26-49:08
//   (con el default de whisper.cpp, 224 tokens);
// - entrevista 6 con large-v3: con 64 tokens, una ventana sin puntuar en el
//   minuto 18 dejo sin puntuacion ni mayusculas el resto de la entrevista.
// En su lugar cada ventana se condiciona solo con el prompt fijo (glosario +
// STYLE_PROMPT, ver buildInitialPrompt), que siempre esta bien puntuado.
constexpr int MAIN_PASS_MAX_TEXT_CTX = 0;

// Frase de estilo que se repite como contexto en cada ventana: whisper imita
// la forma del texto previo, asi que un ejemplo con puntuacion, tildes y
// signos de pregunta sostiene ese estilo durante toda la entrevista. Va al
// final del prompt (lo mas cercano al audio, lo que mas pesa).
constexpr std::string_view STYLE_PROMPT = "Transcripción fiel de una entrevista, con puntuación, tildes y signos de pregunta: ¿cómo funciona? Bien, se lo explico.";

// Segmentos consecutivos con el mismo texto a partir de los cuales se
// considera un bucle de alucinacion. En habla real una misma frase no se
// repite 4 veces seguidas como segmentos separados.
constexpr size_t LOOP_MIN_REPEATS = 4;

// Beam search: mas robusto que greedy (menos palabras inventadas/omitidas),
// pero varias veces mas lento. En GPU se usa en la pasada principal; en CPU
// solo para re-transcribir tramos en bucle (unos minutos, no la entrevista).
constexpr int BEAM_SIZE = 5;

// Tope de tokens del prompt inicial (glosario + STYLE_PROMPT). whisper.cpp
// acota todo el contexto de texto a n_text_ctx/2 = 224 tokens
// (whisper.cpp:6913); 150 deja margen para el marcador de contexto previo.
constexpr int MAX_PROMPT_TOKENS = 150;
constexpr int WHISPER_MAX_TEXT_CTX = 224;

// Parametros del VAD (Silero). Los defaults de whisper.cpp recortan de mas
// para entrevistas: 30ms de padding come el inicio/fin de las palabras y
// 100ms de silencio parte frases en pausas normales de quien piensa lo que
// dice. Valores de partida a validar con el A/B (ver ADR-020).
constexpr float VAD_THRESHOLD = 0.45f;
constexpr int VAD_MIN_SPEECH_MS = 250;
constexpr int VAD_MIN_SILENCE_MS = 400;
constexpr int VAD_SPEECH_PAD_MS = 200;
constexpr float VAD_MAX_SPEECH_S = 30.0f;

// Modelos por encima de este tamaño (medium, large-v3) en CPU son varias
// veces mas lentos que small: se avisa en el log para que no parezca colgado.
constexpr std::uintmax_t LARGE_MODEL_BYTES = 1'000'000'000;

// Frases que whisper inventa en silencios o ruido porque aparecen al final
// de muchos videos subtitulados con los que fue entrenado. Solo se descarta
// un segmento si su texto COMPLETO (normalizado) es una de estas y dura
// poco: una persona real puede decir "gracias" en una entrevista.
constexpr int64_t HALLUCINATION_MAX_CS = 300;
constexpr std::array<std::string_view, 5> HALLUCINATION_PHRASES = {
    "graciasporverelvideo",
    "graciasporver",
    "suscribetealcanal",
    "noolvidessuscribirte",
    "subtitulosrealizadosporlacomunidaddeamaraorg",
};

// Densidad maxima creible de texto por segmento. El habla real en espanol
// ronda 12-18 caracteres por segundo; whisper (large-v3, sobre todo) a veces
// emite segmentos de ~0.1s con una frase entera repetida de lo dicho justo
// antes (entrevista 6, 01:49: tres copias de la pregunta en 100-360ms). El
// margen fijo protege monosilabos ("Sí.") con tiempos muy ajustados. Validado
// sobre los 379 segmentos de esa entrevista: marca solo las 3 alucinaciones.
constexpr double MAX_CHARS_PER_SECOND = 35.0;
constexpr size_t DENSITY_MARGIN_CHARS = 10;

size_t utf8Length(const std::string& text) {
    size_t count = 0;
    for (unsigned char c : text) {
        if ((c & 0xC0) != 0x80) ++count;  // no cuenta bytes de continuacion
    }
    return count;
}

bool isImplausiblyDense(const RawSegment& segment) {
    const double seconds = static_cast<double>(std::max<int64_t>(segment.t1 - segment.t0, 0)) / 100.0;
    return static_cast<double>(utf8Length(segment.text)) > seconds * MAX_CHARS_PER_SECOND + DENSITY_MARGIN_CHARS;
}

// Texto comparable: minusculas, sin puntuacion ni espacios ("Que no se
// cumplen los plazos," == "que no se cumplen los plazos"). Los bytes no
// ASCII (acentos en UTF-8) se conservan tal cual.
std::string normalizeForComparison(const std::string& text) {
    std::string normalized;
    normalized.reserve(text.size());
    for (unsigned char c : text) {
        if (c >= 0x80 || std::isalnum(c)) {
            normalized += static_cast<char>(std::tolower(c));
        }
    }
    return normalized;
}

bool isKnownHallucination(const RawSegment& segment) {
    if (segment.t1 - segment.t0 > HALLUCINATION_MAX_CS) return false;
    const std::string normalized = normalizeForComparison(segment.text);
    if (normalized.empty()) return false;
    // "amara.org" aparece con variantes (con/sin tilde en "subtítulos").
    if (normalized.find("amaraorg") != std::string::npos) return true;
    return std::find(HALLUCINATION_PHRASES.begin(), HALLUCINATION_PHRASES.end(), normalized) != HALLUCINATION_PHRASES.end();
}

struct RepeatRun {
    size_t first;  // indice del primer segmento del bucle (inclusive)
    size_t last;   // indice del ultimo segmento del bucle (inclusive)
};

std::vector<RepeatRun> findRepeatRuns(const std::vector<RawSegment>& segments) {
    std::vector<RepeatRun> runs;
    if (segments.empty()) return runs;

    size_t start = 0;
    std::string startText = normalizeForComparison(segments[0].text);
    for (size_t i = 1; i <= segments.size(); ++i) {
        const std::string text = i < segments.size() ? normalizeForComparison(segments[i].text) : std::string();
        if (i < segments.size() && !text.empty() && text == startText) continue;
        if (i - start >= LOOP_MIN_REPEATS) {
            runs.push_back({start, i - 1});
        }
        start = i;
        startText = text;
    }
    return runs;
}

std::string formatClock(int64_t centiseconds) {
    const int64_t totalSeconds = centiseconds / 100;
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02lld:%02lld",
                  static_cast<long long>(totalSeconds / 60), static_cast<long long>(totalSeconds % 60));
    return std::string(buf);
}

// Sin esto, un audio largo no deja ningun rastro en el log entre
// "Transcribiendo con whisper.cpp" y el resultado final - parece trabado
// aunque este funcionando. Se loguea cada 10% en vez de en cada llamada
// (whisper.cpp invoca este callback muy seguido).
struct ProgressLogState {
    int lastLoggedPercent = -1;
};

void logWhisperProgress(struct whisper_context* /*ctx*/, struct whisper_state* /*state*/, int progress, void* user_data) {
    auto* logState = static_cast<ProgressLogState*>(user_data);
    if (progress - logState->lastLoggedPercent >= 10 || progress == 100) {
        log_event("[WhisperTranscriber][transcribe] Progreso: " + std::to_string(progress) + "%");
        logState->lastLoggedPercent = progress;
    }
}

// whisper.cpp decodifica por token BPE, no por caracter: en audio largo/
// ruidoso puede emitir un token cuyos bytes truncan una secuencia UTF-8
// multibyte a mitad de camino. Eso produce texto con bytes invalidos que
// nlohmann::json::dump() rechaza con type_error.316 - visto en produccion
// con un audio real de mas de 100MB. Se repara reemplazando cualquier
// secuencia invalida por U+FFFD, en vez de dejar que el error tumbe el job.
std::string sanitizeUtf8(const std::string& input) {
    std::string output;
    output.reserve(input.size());
    size_t i = 0;

    while (i < input.size()) {
        const unsigned char c = static_cast<unsigned char>(input[i]);
        int len = 0;
        if ((c & 0x80) == 0x00) len = 1;        // 0xxxxxxx
        else if ((c & 0xE0) == 0xC0) len = 2;   // 110xxxxx
        else if ((c & 0xF0) == 0xE0) len = 3;   // 1110xxxx
        else if ((c & 0xF8) == 0xF0) len = 4;   // 11110xxx

        if (len == 0 || i + len > input.size()) {
            output += "\xEF\xBF\xBD";  // U+FFFD
            ++i;
            continue;
        }

        bool validContinuation = true;
        for (int k = 1; k < len; ++k) {
            const unsigned char cc = static_cast<unsigned char>(input[i + k]);
            if ((cc & 0xC0) != 0x80) {
                validContinuation = false;
                break;
            }
        }

        if (validContinuation) {
            output.append(input, i, len);
        } else {
            output += "\xEF\xBF\xBD";
        }
        i += len;
    }

    return output;
}

std::string trimSpaces(const std::string& text) {
    const size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const size_t last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

// Dispositivo GPU que ggml ve en este build (Vulkan), o nullptr si no hay
// ninguno (build sin backend GPU, o maquina sin driver Vulkan).
ggml_backend_dev_t findGpuDevice() {
    return ggml_backend_dev_by_type(GGML_BACKEND_DEVICE_TYPE_GPU);
}

}  // namespace

WhisperTranscriber::WhisperTranscriber(WhisperOptions options) : m_options(std::move(options)) {}

WhisperTranscriber::~WhisperTranscriber() {
    if (m_context) {
        whisper_free(m_context);
    }
}

void WhisperTranscriber::ensureModelLoaded() {
    if (m_context) {
        return;
    }

    if (!std::filesystem::exists(m_options.modelPath)) {
        throw std::runtime_error(
            "Modelo de whisper.cpp no encontrado en '" + m_options.modelPath +
            "'. Descargalo con models/download-ggml-model.sh (whisper.cpp) o desde "
            "https://huggingface.co/ggerganov/whisper.cpp y coloca el .bin en esa ruta "
            "(configurable con la env var WHISPER_MODEL_PATH). Ver 'whisper.cpp Architecture' en la vault.");
    }

    ggml_backend_dev_t gpuDevice = nullptr;
    if (m_options.gpuMode != GpuMode::Off) {
        gpuDevice = findGpuDevice();
        if (!gpuDevice) {
            log_event("[WhisperTranscriber][ensureModelLoaded] No se detecto GPU compatible (Vulkan), se usa CPU");
        }
    }

    auto load = [this](bool useGpu) {
        whisper_context_params params = whisper_context_default_params();
        params.use_gpu = useGpu;
        // El default de whisper.cpp 1.8.6 trae flash_attn=true. La ruta CPU
        // de flash attention crasheo en este build (MinGW estatico, acceso a
        // memoria invalido sin excepcion capturable): solo se activa en GPU.
        params.flash_attn = useGpu;
        return whisper_init_from_file_with_params(m_options.modelPath.c_str(), params);
    };

    if (gpuDevice) {
        m_context = load(true);
        if (m_context) {
            m_gpuActive = true;
            log_event(std::string("[WhisperTranscriber][ensureModelLoaded] Modelo cargado en GPU: ") +
                      ggml_backend_dev_description(gpuDevice) + " (" + m_options.modelPath + ")");
            return;
        }
        log_event("[WhisperTranscriber][ensureModelLoaded] No se pudo cargar el modelo en GPU, se reintenta en CPU");
    }

    m_context = load(false);
    if (!m_context) {
        throw std::runtime_error("whisper.cpp no pudo cargar el modelo: " + m_options.modelPath);
    }
    m_gpuActive = false;
    log_event("[WhisperTranscriber][ensureModelLoaded] Modelo cargado en CPU (" + m_options.modelPath + ")");

    std::error_code ec;
    if (std::filesystem::file_size(m_options.modelPath, ec) > LARGE_MODEL_BYTES && !ec) {
        log_event("[WhisperTranscriber][ensureModelLoaded] Modelo grande en CPU: la transcripcion sera varias veces mas "
                  "lenta. Para CPU conviene WHISPER_MODEL_PATH=./models/ggml-small.bin");
    }
}

std::string WhisperTranscriber::buildInitialPrompt(const std::vector<std::string>& keywords, int& promptTokens) {
    std::vector<whisper_token> tokens(MAX_PROMPT_TOKENS * 2);
    auto countTokens = [&](const std::string& text) {
        return whisper_tokenize(m_context, text.c_str(), tokens.data(), static_cast<int>(tokens.size()));
    };

    const std::string style(STYLE_PROMPT);
    std::string prompt = style;
    promptTokens = countTokens(prompt);

    // "Glosario: a, b, c. <estilo>": se agregan terminos mientras entren en
    // MAX_PROMPT_TOKENS junto con la frase de estilo.
    std::string glossary;
    size_t used = 0;
    for (const auto& keyword : keywords) {
        const std::string candidateGlossary = (glossary.empty() ? "Glosario: " : glossary.substr(0, glossary.size() - 1) + ", ") + keyword + ".";
        const std::string candidate = candidateGlossary + " " + style;
        const int n = countTokens(candidate);
        if (n < 0 || n > MAX_PROMPT_TOKENS) break;
        glossary = candidateGlossary;
        prompt = candidate;
        promptTokens = n;
        ++used;
    }
    if (used < keywords.size()) {
        log_event("[WhisperTranscriber][transcribe] Glosario demasiado largo para whisper: se usan " + std::to_string(used) +
                  " de " + std::to_string(keywords.size()) + " terminos (la sanitizacion posterior usa todos)");
    }
    return prompt;
}

std::vector<TranscriptSegment> WhisperTranscriber::transcribe(const std::string& audioPath,
                                                              const TranscriptionOptions& options) {
    std::lock_guard<std::mutex> lock(m_mutex);

    ensureModelLoaded();

    const std::vector<float> samples = hermes::audio::readNormalizedWav(audioPath);
    if (samples.empty()) {
        throw std::runtime_error("El audio normalizado esta vacio: " + audioPath);
    }

    const bool useVad = std::filesystem::exists(m_options.vadModelPath);
    if (!useVad) {
        log_event("[WhisperTranscriber][transcribe] Modelo VAD no encontrado en '" + m_options.vadModelPath +
                  "', se transcribe sin VAD (mayor riesgo de repeticiones alucinadas en silencios). "
                  "Configurable con WHISPER_VAD_MODEL_PATH.");
    }

    // Prompt fijo (glosario ADR-018 + estilo): carry_initial_prompt lo repite
    // en cada ventana de 30s, no solo en la primera. n_max_text_ctx es el
    // presupuesto TOTAL de contexto (prompt + texto previo), asi que se le
    // suma el prompt para no comerse el contexto dinamico.
    int promptTokens = 0;
    const std::string initialPrompt = buildInitialPrompt(options.keywords, promptTokens);
    auto applyPrompt = [&](whisper_full_params& p, int dynamicContext) {
        if (promptTokens <= 0) {
            p.n_max_text_ctx = dynamicContext;
            return;
        }
        p.initial_prompt = initialPrompt.c_str();
        p.carry_initial_prompt = true;
        p.n_max_text_ctx = std::min(WHISPER_MAX_TEXT_CTX, promptTokens + 1 + dynamicContext);
    };

    // Pasada principal: beam search en GPU (fidelidad), greedy en CPU
    // (velocidad). Sin texto previo en ambos casos (ver MAIN_PASS_MAX_TEXT_CTX).
    whisper_full_params params = whisper_full_default_params(m_gpuActive ? WHISPER_SAMPLING_BEAM_SEARCH : WHISPER_SAMPLING_GREEDY);
    if (m_gpuActive) {
        params.beam_search.beam_size = BEAM_SIZE;
    }
    applyPrompt(params, MAIN_PASS_MAX_TEXT_CTX);
    ProgressLogState progressLogState;
    params.progress_callback = logWhisperProgress;
    params.progress_callback_user_data = &progressLogState;
    log_event(std::string("[WhisperTranscriber][transcribe] Pasada principal: ") +
              (m_gpuActive ? "GPU, beam search " + std::to_string(BEAM_SIZE) : std::string("CPU, greedy")) +
              (useVad ? ", con VAD" : ", sin VAD"));

    std::vector<RawSegment> segments = runWhisper(params, samples.data(), samples.size(), 0, useVad);

    // Reparacion de bucles: se recorre de atras hacia adelante para que
    // reemplazar un tramo no invalide los indices de los tramos anteriores.
    const auto runs = findRepeatRuns(segments);
    for (auto it = runs.rbegin(); it != runs.rend(); ++it) {
        const int64_t t0 = segments[it->first].t0;
        const int64_t t1 = segments[it->last].t1;
        const std::string range = formatClock(t0) + "-" + formatClock(t1);
        log_event("[WhisperTranscriber][transcribe] Bucle detectado (" + std::to_string(it->last - it->first + 1) +
                  " segmentos repetidos, " + range + "), re-transcribiendo el tramo");

        // Sin texto previo (contexto dinamico 0): el bucle se alimentaba
        // justamente de eso. El glosario, si hay, se mantiene.
        whisper_full_params retryParams = whisper_full_default_params(WHISPER_SAMPLING_BEAM_SEARCH);
        retryParams.beam_search.beam_size = BEAM_SIZE;
        applyPrompt(retryParams, 0);

        const size_t firstSample = std::min(samples.size(), static_cast<size_t>(t0 * SAMPLES_PER_CENTISECOND));
        const size_t lastSample = std::min(samples.size(), static_cast<size_t>(t1 * SAMPLES_PER_CENTISECOND));
        std::vector<RawSegment> retry;
        if (lastSample > firstSample) {
            retry = runWhisper(retryParams, samples.data() + firstSample, lastSample - firstSample, t0, useVad);
        }

        std::vector<RawSegment> replacement;
        if (!retry.empty() && findRepeatRuns(retry).empty()) {
            log_event("[WhisperTranscriber][transcribe] Tramo " + range + " recuperado (" + std::to_string(retry.size()) + " segmentos)");
            replacement = std::move(retry);
        } else {
            // Se conserva la primera aparicion (probablemente dicha de verdad)
            // y se marca el resto, para que el investigador sepa que escuchar.
            const RawSegment firstOccurrence = segments[it->first];
            const std::string lost = formatClock(firstOccurrence.t1) + "-" + formatClock(t1);
            log_event("[WhisperTranscriber][transcribe] No se pudo recuperar el tramo, se marca como no transcrito: " + lost);
            replacement.push_back(firstOccurrence);
            replacement.push_back({firstOccurrence.t1, t1, "[audio no transcrito " + lost + "]", {}});
        }

        segments.erase(segments.begin() + it->first, segments.begin() + it->last + 1);
        segments.insert(segments.begin() + it->first, replacement.begin(), replacement.end());
    }

    std::vector<TranscriptSegment> result;
    result.reserve(segments.size());
    for (auto& segment : segments) {
        if (isKnownHallucination(segment)) {
            log_event("[WhisperTranscriber][transcribe] Segmento descartado por alucinacion conocida (" +
                      formatClock(segment.t0) + "): " + segment.text);
            continue;
        }
        if (isImplausiblyDense(segment)) {
            log_event("[WhisperTranscriber][transcribe] Segmento descartado por densidad imposible (" +
                      std::to_string(utf8Length(segment.text)) + " caracteres en " +
                      std::to_string((segment.t1 - segment.t0) * 10) + "ms, " + formatClock(segment.t0) + "): " + segment.text);
            continue;
        }
        result.push_back({segment.t0 * 10, segment.t1 * 10, std::move(segment.text), std::move(segment.words)});
    }
    return result;
}

std::vector<RawSegment> WhisperTranscriber::runWhisper(whisper_full_params params,
                                                       const float* samples,
                                                       size_t sampleCount,
                                                       int64_t offsetCs,
                                                       bool useVad) {
    params.language = m_options.language.c_str();
    params.print_progress = false;
    params.print_realtime = false;
    params.print_special = false;
    params.print_timestamps = false;
    // No conviene paralelizar de mas en una laptop de researcher (mismo
    // criterio que WORKER_POOL_SIZE=1 por defecto en Sprint 4). Con GPU los
    // hilos de CPU pesan poco igual.
    params.n_threads = static_cast<int>(std::min(4u, std::thread::hardware_concurrency()));
    // Descarta tokens de no-habla ("[Música]", "♪", "(risas)"): no son
    // contenido de la entrevista y confunden la diarizacion/correccion.
    params.suppress_nst = true;
    // Tiempos por token, para ubicar cada palabra (ver extractWords).
    params.token_timestamps = true;
    // whisper.cpp remapea los timestamps de SEGMENTO al audio original cuando
    // usa VAD (no los de token - ver extractWords).
    params.vad = useVad;
    params.vad_model_path = useVad ? m_options.vadModelPath.c_str() : nullptr;
    if (useVad) {
        params.vad_params = whisper_vad_default_params();
        params.vad_params.threshold = VAD_THRESHOLD;
        params.vad_params.min_speech_duration_ms = VAD_MIN_SPEECH_MS;
        params.vad_params.min_silence_duration_ms = VAD_MIN_SILENCE_MS;
        params.vad_params.speech_pad_ms = VAD_SPEECH_PAD_MS;
        params.vad_params.max_speech_duration_s = VAD_MAX_SPEECH_S;
    }

    if (whisper_full(m_context, params, samples, static_cast<int>(sampleCount)) != 0) {
        throw std::runtime_error("whisper.cpp fallo al transcribir");
    }

    const int segmentCount = whisper_full_n_segments(m_context);
    std::vector<RawSegment> segments;
    segments.reserve(segmentCount);
    for (int i = 0; i < segmentCount; ++i) {
        const int64_t t0 = offsetCs + whisper_full_get_segment_t0(m_context, i);
        const int64_t t1 = offsetCs + whisper_full_get_segment_t1(m_context, i);
        segments.push_back({t0, t1, sanitizeUtf8(whisper_full_get_segment_text(m_context, i)), extractWords(i, t0, t1)});
    }
    return segments;
}

std::vector<TranscriptWord> WhisperTranscriber::extractWords(int segmentIndex, int64_t segT0Cs, int64_t segT1Cs) {
    // Agrupa tokens BPE en palabras: un token que empieza con espacio abre
    // una palabra nueva. Los tokens especiales (timestamps, [_BEG_], etc.)
    // tienen id >= eot y no son texto.
    struct PendingWord {
        std::string text;
        int64_t rawT0;
        int64_t rawT1;
    };
    std::vector<PendingWord> pending;
    const whisper_token eot = whisper_token_eot(m_context);
    const int tokenCount = whisper_full_n_tokens(m_context, segmentIndex);
    for (int k = 0; k < tokenCount; ++k) {
        const whisper_token_data data = whisper_full_get_token_data(m_context, segmentIndex, k);
        if (data.id >= eot) continue;
        const std::string tokenText = whisper_full_get_token_text(m_context, segmentIndex, k);
        if (tokenText.empty()) continue;
        if (pending.empty() || tokenText.front() == ' ') {
            pending.push_back({tokenText, data.t0, data.t1});
        } else {
            pending.back().text += tokenText;
            pending.back().rawT1 = data.t1;
        }
    }

    std::vector<TranscriptWord> words;
    if (pending.empty()) return words;

    // Los tiempos de token NO estan remapeados por el VAD (whisper.cpp 1.8.6
    // solo remapea t0/t1 de segmento): estan en el tiempo del audio ya sin
    // silencios. Se proyectan linealmente sobre el rango del segmento, que
    // si esta en tiempo original. Sin VAD la proyeccion es casi la identidad.
    // Es una aproximacion: alcanza para decidir de que lado de un cambio de
    // hablante cae cada palabra.
    const int64_t rawStart = pending.front().rawT0;
    const int64_t rawEnd = std::max(pending.back().rawT1, rawStart);
    const int64_t rawSpan = rawEnd - rawStart;
    const int64_t segSpan = std::max<int64_t>(segT1Cs - segT0Cs, 0);
    const size_t n = pending.size();
    words.reserve(n);
    for (size_t w = 0; w < n; ++w) {
        int64_t startCs;
        int64_t endCs;
        if (rawSpan > 0) {
            startCs = segT0Cs + (std::clamp(pending[w].rawT0, rawStart, rawEnd) - rawStart) * segSpan / rawSpan;
            endCs = segT0Cs + (std::clamp(pending[w].rawT1, rawStart, rawEnd) - rawStart) * segSpan / rawSpan;
        } else {
            // Sin tiempos de token utiles: reparto uniforme.
            startCs = segT0Cs + static_cast<int64_t>(w) * segSpan / static_cast<int64_t>(n);
            endCs = segT0Cs + static_cast<int64_t>(w + 1) * segSpan / static_cast<int64_t>(n);
        }
        const std::string text = trimSpaces(sanitizeUtf8(pending[w].text));
        if (text.empty()) continue;
        words.push_back({startCs * 10, std::max(startCs, endCs) * 10, text});
    }
    return words;
}

}  // namespace hermes::transcription

#include "../include/WhisperTranscriber.h"
#include "../../api/include/logger.h"

#include <whisper.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <thread>
#include <vector>

namespace hermes::transcription {

namespace {

struct WavData {
    std::vector<float> samples;  // mono, normalizado a [-1, 1]
};

// Lee el WAV canonico de 44 bytes que escribe FfmpegAudioNormalizer
// (PCM 16-bit, 16kHz, mono). No es un parser WAV general: si algun dia
// IAudioNormalizer cambia de formato de salida, esto tiene que cambiar
// junto con el.
WavData readNormalizedWav(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        throw std::runtime_error("No se pudo abrir el audio normalizado: " + path);
    }

    char riffTag[4];
    char waveTag[4];
    in.read(riffTag, 4);
    in.seekg(4, std::ios::cur);  // tamaño total RIFF, no lo necesitamos
    in.read(waveTag, 4);
    if (std::string(riffTag, 4) != "RIFF" || std::string(waveTag, 4) != "WAVE") {
        throw std::runtime_error("El audio normalizado no es un WAV valido: " + path);
    }

    // Busca el chunk "data" en vez de asumir el offset fijo 44: es barato
    // y tolera que el header traiga chunks extra si el normalizador cambia.
    char chunkId[4];
    uint32_t chunkSize = 0;
    std::streamoff dataOffset = -1;
    while (in.read(chunkId, 4)) {
        in.read(reinterpret_cast<char*>(&chunkSize), 4);
        if (std::string(chunkId, 4) == "data") {
            dataOffset = in.tellg();
            break;
        }
        in.seekg(chunkSize, std::ios::cur);
    }

    if (dataOffset < 0) {
        throw std::runtime_error("El audio normalizado no tiene chunk 'data': " + path);
    }

    const size_t sampleCount = chunkSize / sizeof(int16_t);
    std::vector<int16_t> raw(sampleCount);
    in.read(reinterpret_cast<char*>(raw.data()), chunkSize);

    WavData wav;
    wav.samples.resize(sampleCount);
    for (size_t i = 0; i < sampleCount; ++i) {
        wav.samples[i] = static_cast<float>(raw[i]) / 32768.0f;
    }
    return wav;
}

// whisper_full_get_segment_t0/t1 devuelven el tiempo en centisegundos
// (unidades de 10ms), no milisegundos directos - ver whisper.cpp
// Architecture en la vault.
std::string formatTimestamp(int64_t centiseconds) {
    const int64_t totalMs = centiseconds * 10;
    const int64_t totalSeconds = totalMs / 1000;
    const int64_t hours = totalSeconds / 3600;
    const int64_t minutes = (totalSeconds % 3600) / 60;
    const int64_t seconds = totalSeconds % 60;

    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02lld:%02lld:%02lld",
                  static_cast<long long>(hours), static_cast<long long>(minutes), static_cast<long long>(seconds));
    return std::string(buf);
}

constexpr int SAMPLE_RATE = 16000;
constexpr int64_t SAMPLES_PER_CENTISECOND = SAMPLE_RATE / 100;

// Tokens de texto previo con los que se condiciona cada ventana de 30s en
// la pasada principal. El default de whisper.cpp (16384, acotado a 224) es
// lo que deja que una repeticion se autoalimente hasta el final del audio:
// entrevista 13, "que no se cumplen los plazos" x114 durante 43:26-49:08.
// Un contexto corto conserva algo de continuidad sin arrastrar el bucle.
constexpr int MAIN_PASS_MAX_TEXT_CTX = 64;

// Segmentos consecutivos con el mismo texto a partir de los cuales se
// considera un bucle de alucinacion. En habla real una misma frase no se
// repite 4 veces seguidas como segmentos separados.
constexpr size_t LOOP_MIN_REPEATS = 4;

// Beam search solo en la re-transcripcion de tramos en bucle: mas robusto
// que greedy, pero varias veces mas lento - inaceptable para la entrevista
// entera en CPU, aceptable para unos minutos.
constexpr int RETRY_BEAM_SIZE = 5;

// Tope de tokens del glosario como initial_prompt. whisper.cpp acota todo el
// contexto de texto a n_text_ctx/2 = 224 tokens (whisper.cpp:6913); con 150
// para el glosario quedan los 64 de MAIN_PASS_MAX_TEXT_CTX + el marcador.
constexpr int MAX_PROMPT_TOKENS = 150;
constexpr int WHISPER_MAX_TEXT_CTX = 224;

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

struct WhisperContextDeleter {
    void operator()(whisper_context* ctx) const {
        if (ctx) whisper_free(ctx);
    }
};

// Sin esto, un audio largo (la transcripcion de una entrevista real de
// 70min tarda ~12 min) no deja ningun rastro en el log entre "Transcribiendo
// con whisper.cpp" y el resultado final - parece trabado aunque este
// funcionando. Se loguea cada 10% en vez de en cada llamada (whisper.cpp
// invoca este callback muy seguido).
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
// ruidoso (o con modelos chicos como ggml-tiny) puede emitir un token cuyos
// bytes truncan una secuencia UTF-8 multibyte a mitad de camino. Eso
// produce texto con bytes invalidos que nlohmann::json::dump() rechaza con
// type_error.316 ("invalid UTF-8 byte") - visto en produccion con un audio
// real de mas de 100MB. Se repara reemplazando cualquier secuencia
// invalida por el caracter de reemplazo U+FFFD, en vez de dejar que
// el error tumbe el job entero.
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

}  // namespace

WhisperTranscriber::WhisperTranscriber(std::string modelPath, std::string language, std::string vadModelPath)
    : m_modelPath(std::move(modelPath)), m_language(std::move(language)), m_vadModelPath(std::move(vadModelPath)) {}

WhisperTranscriber::~WhisperTranscriber() {
    if (m_context) {
        whisper_free(m_context);
    }
}

void WhisperTranscriber::ensureModelLoaded() {
    if (m_context) {
        return;
    }

    if (!std::filesystem::exists(m_modelPath)) {
        throw std::runtime_error(
            "Modelo de whisper.cpp no encontrado en '" + m_modelPath +
            "'. Descargalo con models/download-ggml-model.sh (whisper.cpp) o desde "
            "https://huggingface.co/ggerganov/whisper.cpp y coloca el .bin en esa ruta "
            "(configurable con la env var WHISPER_MODEL_PATH). Ver 'whisper.cpp Architecture' en la vault.");
    }

    whisper_context_params params = whisper_context_default_params();
    params.use_gpu = false;  // build sin cuda/metal/vulkan (ver vcpkg.json) - CPU only
    // El default de whisper.cpp 1.8.6 trae flash_attn=true incluso sin GPU.
    // La ruta CPU de flash attention crasheo en este build (MinGW estatico,
    // sin excepcion capturable - acceso a memoria invalido). Desactivado:
    // preferimos correcto sobre rapido para un researcher local.
    params.flash_attn = false;

    m_context = whisper_init_from_file_with_params(m_modelPath.c_str(), params);
    if (!m_context) {
        throw std::runtime_error("whisper.cpp no pudo cargar el modelo: " + m_modelPath);
    }
}

std::string WhisperTranscriber::buildGlossaryPrompt(const std::vector<std::string>& keywords, int& promptTokens) {
    promptTokens = 0;
    std::string prompt;
    std::vector<whisper_token> tokens(MAX_PROMPT_TOKENS * 2);
    size_t used = 0;
    for (const auto& keyword : keywords) {
        const std::string candidate = (prompt.empty() ? "Glosario: " : prompt.substr(0, prompt.size() - 1) + ", ") + keyword + ".";
        const int n = whisper_tokenize(m_context, candidate.c_str(), tokens.data(), static_cast<int>(tokens.size()));
        if (n < 0 || n > MAX_PROMPT_TOKENS) break;
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

    WavData wav = readNormalizedWav(audioPath);
    if (wav.samples.empty()) {
        throw std::runtime_error("El audio normalizado esta vacio: " + audioPath);
    }

    const bool useVad = std::filesystem::exists(m_vadModelPath);
    if (!useVad) {
        log_event("[WhisperTranscriber][transcribe] Modelo VAD no encontrado en '" + m_vadModelPath +
                  "', se transcribe sin VAD (mayor riesgo de repeticiones alucinadas en silencios). "
                  "Configurable con WHISPER_VAD_MODEL_PATH.");
    }

    // Glosario (ADR-018): carry_initial_prompt lo repite en cada ventana de
    // 30s, no solo en la primera. n_max_text_ctx es el presupuesto TOTAL de
    // contexto (glosario + texto previo), asi que se le suma el glosario
    // para no comerse el contexto dinamico.
    int promptTokens = 0;
    const std::string glossaryPrompt = buildGlossaryPrompt(options.keywords, promptTokens);
    auto applyGlossary = [&](whisper_full_params& p, int dynamicContext) {
        if (promptTokens == 0) {
            p.n_max_text_ctx = dynamicContext;
            return;
        }
        p.initial_prompt = glossaryPrompt.c_str();
        p.carry_initial_prompt = true;
        p.n_max_text_ctx = std::min(WHISPER_MAX_TEXT_CTX, promptTokens + 1 + dynamicContext);
    };

    // Pasada principal: greedy, contexto previo acotado.
    whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    applyGlossary(params, MAIN_PASS_MAX_TEXT_CTX);
    ProgressLogState progressLogState;
    params.progress_callback = logWhisperProgress;
    params.progress_callback_user_data = &progressLogState;

    std::vector<RawSegment> segments = runWhisper(params, wav.samples.data(), wav.samples.size(), 0, useVad);

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
        retryParams.beam_search.beam_size = RETRY_BEAM_SIZE;
        applyGlossary(retryParams, 0);

        const size_t firstSample = std::min(wav.samples.size(), static_cast<size_t>(t0 * SAMPLES_PER_CENTISECOND));
        const size_t lastSample = std::min(wav.samples.size(), static_cast<size_t>(t1 * SAMPLES_PER_CENTISECOND));
        std::vector<RawSegment> retry;
        if (lastSample > firstSample) {
            retry = runWhisper(retryParams, wav.samples.data() + firstSample, lastSample - firstSample, t0, useVad);
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
            replacement.push_back({firstOccurrence.t1, t1, "[audio no transcrito " + lost + "]"});
        }

        segments.erase(segments.begin() + it->first, segments.begin() + it->last + 1);
        segments.insert(segments.begin() + it->first, replacement.begin(), replacement.end());
    }

    std::vector<TranscriptSegment> result;
    result.reserve(segments.size());
    for (const auto& segment : segments) {
        result.push_back({formatTimestamp(segment.t0), formatTimestamp(segment.t1), segment.text});
    }
    return result;
}

std::vector<RawSegment> WhisperTranscriber::runWhisper(whisper_full_params params,
                                                       const float* samples,
                                                       size_t sampleCount,
                                                       int64_t offsetCs,
                                                       bool useVad) {
    params.language = m_language.c_str();
    params.print_progress = false;
    params.print_realtime = false;
    params.print_special = false;
    params.print_timestamps = false;
    // No conviene paralelizar de mas en una laptop de researcher (mismo
    // criterio que WORKER_POOL_SIZE=1 por defecto en Sprint 4).
    params.n_threads = static_cast<int>(std::min(4u, std::thread::hardware_concurrency()));
    // whisper.cpp remapea los timestamps al audio original cuando usa VAD.
    params.vad = useVad;
    params.vad_model_path = useVad ? m_vadModelPath.c_str() : nullptr;

    if (whisper_full(m_context, params, samples, static_cast<int>(sampleCount)) != 0) {
        throw std::runtime_error("whisper.cpp fallo al transcribir");
    }

    const int segmentCount = whisper_full_n_segments(m_context);
    std::vector<RawSegment> segments;
    segments.reserve(segmentCount);
    for (int i = 0; i < segmentCount; ++i) {
        segments.push_back({
            offsetCs + whisper_full_get_segment_t0(m_context, i),
            offsetCs + whisper_full_get_segment_t1(m_context, i),
            sanitizeUtf8(whisper_full_get_segment_text(m_context, i)),
        });
    }
    return segments;
}

}  // namespace hermes::transcription

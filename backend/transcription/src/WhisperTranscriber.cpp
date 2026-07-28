#include "../include/WhisperTranscriber.h"

#include <whisper.h>

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

struct WhisperContextDeleter {
    void operator()(whisper_context* ctx) const {
        if (ctx) whisper_free(ctx);
    }
};

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

WhisperTranscriber::WhisperTranscriber(std::string modelPath, std::string language)
    : m_modelPath(std::move(modelPath)), m_language(std::move(language)) {}

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

std::vector<TranscriptSegment> WhisperTranscriber::transcribe(const std::string& audioPath) {
    std::lock_guard<std::mutex> lock(m_mutex);

    ensureModelLoaded();

    WavData wav = readNormalizedWav(audioPath);
    if (wav.samples.empty()) {
        throw std::runtime_error("El audio normalizado esta vacio: " + audioPath);
    }

    whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.language = m_language.c_str();
    params.print_progress = false;
    params.print_realtime = false;
    params.print_special = false;
    params.print_timestamps = false;
    // No conviene paralelizar de mas en una laptop de researcher (mismo
    // criterio que WORKER_POOL_SIZE=1 por defecto en Sprint 4).
    params.n_threads = static_cast<int>(std::min(4u, std::thread::hardware_concurrency()));

    if (whisper_full(m_context, params, wav.samples.data(), static_cast<int>(wav.samples.size())) != 0) {
        throw std::runtime_error("whisper.cpp fallo al transcribir: " + audioPath);
    }

    const int segmentCount = whisper_full_n_segments(m_context);
    std::vector<TranscriptSegment> segments;
    segments.reserve(segmentCount);

    for (int i = 0; i < segmentCount; ++i) {
        TranscriptSegment segment;
        segment.start = formatTimestamp(whisper_full_get_segment_t0(m_context, i));
        segment.end = formatTimestamp(whisper_full_get_segment_t1(m_context, i));
        segment.text = sanitizeUtf8(whisper_full_get_segment_text(m_context, i));
        segments.push_back(std::move(segment));
    }

    return segments;
}

}  // namespace hermes::transcription

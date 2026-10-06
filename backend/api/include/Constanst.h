#ifndef CONSTANST_H
#define CONSTANST_H

#include <string_view>
#include <array>
#include <algorithm>
#include <cctype>
#include <cstddef>

namespace Config {
    // Definimos cada formato individualmente por si necesitas usarlos por separado
    inline constexpr std::string_view MIME_MP3       = "audio/mpeg";
    inline constexpr std::string_view MIME_WAV       = "audio/wav";
    inline constexpr std::string_view MIME_OGG       = "audio/ogg";
    inline constexpr std::string_view MIME_M4A       = "audio/x-m4a";
    // Video: solo se usa su pista de audio. La extrae FfmpegAudioNormalizer en
    // el job, y el video original se borra despues (ver ADR en .ai/DECISIONS.md).
    inline constexpr std::string_view MIME_MP4       = "video/mp4";
    inline constexpr std::string_view MIME_MULTIPART = "multipart/form-data";

    // Formatos validos para el archivo enviado dentro del multipart.
    // MIME_MULTIPART queda fuera: es el Content-Type del request (el "sobre"),
    // no el del archivo real, y no debe usarse para validar el contenido.
    inline constexpr std::array<std::string_view, 5> FORMATOS_PERMITIDOS = {
        MIME_MP3,
        MIME_WAV,
        MIME_OGG,
        MIME_M4A,
        MIME_MP4
    };

    // Tope de subida (1 GiB): cubre ~1h de video de Zoom/Meet. Crow mantiene
    // el body completo en memoria y el multipart se copia varias veces, asi
    // que sin tope un video grande puede agotar la RAM. Mantener sincronizado
    // con MAX_UPLOAD_BYTES en frontend/src/api/interviews.ts.
    inline constexpr std::size_t MAX_UPLOAD_BYTES = 1024ULL * 1024ULL * 1024ULL;

    // Glosario de palabras clave por entrevista (ADR-018). Mantener
    // sincronizado con frontend/src/api/interviews.ts.
    inline constexpr std::size_t MAX_KEYWORDS = 100;
    inline constexpr std::size_t MAX_KEYWORD_LENGTH = 80;

    // Extensiones (tal como quedan en interviews_audio.interview_audio_format)
    // de contenedores de video cuyo original no se retiene tras extraer el audio.
    inline constexpr std::array<std::string_view, 1> VIDEO_EXTENSIONS = { ".mp4" };

    inline bool isVideoExtension(std::string_view ext) {
        return std::any_of(VIDEO_EXTENSIONS.begin(), VIDEO_EXTENSIONS.end(), [&](std::string_view video) {
            return ext.size() == video.size()
                && std::equal(ext.begin(), ext.end(), video.begin(), [](char a, char b) {
                       return std::tolower(static_cast<unsigned char>(a)) == b;
                   });
        });
    }

    inline constexpr std::string_view UPLOAD_DIRECTORY = "./uploads";

    // Raiz de los artefactos generados por el pipeline de procesamiento
    // (Sprint 5+): audio normalizado, transcripcion cruda, resultado final.
    // Estructura: storage/interviews/<interview_id>/...
    inline constexpr std::string_view STORAGE_DIRECTORY = "./storage";

    // Ruta al modelo ggml de whisper.cpp, configurable via env var
    // WHISPER_MODEL_PATH. No se descarga automaticamente (ver whisper.cpp
    // Architecture en la vault): el researcher lo coloca a mano.
    // "large-v3" (~3.1GB): la mayor fidelidad disponible para espanol real
    // (entrevistas largas/ruidosas, vocabulario de dominio). Pensado para
    // correr en GPU (Vulkan, ver ADR-020); en una maquina solo CPU conviene
    // WHISPER_MODEL_PATH=./models/ggml-small.bin (large es varias veces mas lento).
    inline constexpr std::string_view DEFAULT_WHISPER_MODEL_PATH = "./models/ggml-large-v3.bin";
    // VAD (Silero) de whisper.cpp: salta silencios/ruido, donde whisper suele
    // empezar a alucinar repeticiones. Opcional: sin el archivo, no hay VAD.
    inline constexpr std::string_view DEFAULT_WHISPER_VAD_MODEL_PATH = "./models/ggml-silero-v5.1.2.bin";

    // Diarizacion acustica (sherpa-onnx, ver ADR-021). Todo opcional: sin
    // estos archivos se transcribe igual, sin identificar hablantes.
    // Carpeta con sherpa-onnx-c-api.dll + onnxruntime.dll (release v1.13.8).
    inline constexpr std::string_view DEFAULT_DIARIZATION_LIB_DIR = "./sherpa-onnx";
    inline constexpr std::string_view DEFAULT_DIARIZATION_SEGMENTATION_MODEL = "./models/sherpa-onnx-pyannote-segmentation-3-0.onnx";
    // Elegido en el spike sobre una entrevista real de 36 min: mismo resultado
    // que wespeaker-resnet34 (367/369 segmentos coinciden) y ~30% mas rapido.
    inline constexpr std::string_view DEFAULT_DIARIZATION_EMBEDDING_MODEL = "./models/3dspeaker_speech_campplus_sv_zh_en_16k-common_advanced.onnx";
}
#endif // CONSTANST_H
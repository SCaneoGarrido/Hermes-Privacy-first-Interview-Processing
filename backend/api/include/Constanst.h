#ifndef CONSTANST_H
#define CONSTANST_H

#include <string_view>
#include <array>

namespace Config {
    // Definimos cada formato individualmente por si necesitas usarlos por separado
    inline constexpr std::string_view MIME_MP3       = "audio/mpeg";
    inline constexpr std::string_view MIME_WAV       = "audio/wav";
    inline constexpr std::string_view MIME_OGG       = "audio/ogg";
    inline constexpr std::string_view MIME_M4A       = "audio/x-m4a";
    inline constexpr std::string_view MIME_MULTIPART = "multipart/form-data";

    // Formatos de audio validos para el archivo enviado dentro del multipart.
    // MIME_MULTIPART queda fuera: es el Content-Type del request (el "sobre"),
    // no el del archivo real, y no debe usarse para validar el contenido.
    inline constexpr std::array<std::string_view, 4> FORMATOS_AUDIO_PERMITIDOS = {
        MIME_MP3,
        MIME_WAV,
        MIME_OGG,
        MIME_M4A
    };

    inline constexpr std::string_view UPLOAD_DIRECTORY = "./uploads";

    // Raiz de los artefactos generados por el pipeline de procesamiento
    // (Sprint 5+): audio normalizado, transcripcion cruda, resultado final.
    // Estructura: storage/interviews/<interview_id>/...
    inline constexpr std::string_view STORAGE_DIRECTORY = "./storage";

    // Ruta al modelo ggml de whisper.cpp, configurable via env var
    // WHISPER_MODEL_PATH. No se descarga automaticamente (ver whisper.cpp
    // Architecture en la vault): el researcher lo coloca a mano.
    // "small" (~466MB): mejor equilibrio calidad/velocidad en CPU que
    // "tiny" para espanol real (entrevistas largas/ruidosas) sin llegar a
    // la lentitud de "medium" en una laptop sin GPU.
    inline constexpr std::string_view DEFAULT_WHISPER_MODEL_PATH = "./models/ggml-small.bin";
}
#endif // CONSTANST_H
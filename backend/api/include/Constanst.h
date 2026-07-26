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
}
#endif // CONSTANST_H
#ifndef AUDIO_SIGNATURE_H
#define AUDIO_SIGNATURE_H

#include "Constanst.h"
#include <string_view>

// El Content-Type del multipart lo declara el cliente (Postman, curl, etc.)
// y no garantiza nada sobre el contenido real: alcanza con setear el header
// a mano para que un archivo de texto pase como si fuera audio/wav. Esto
// valida los primeros bytes contra la firma real del formato declarado.
namespace AudioSignature {

    inline bool matchesWav(std::string_view content) {
        return content.size() >= 12
            && content.substr(0, 4) == "RIFF"
            && content.substr(8, 4) == "WAVE";
    }

    inline bool matchesOgg(std::string_view content) {
        return content.size() >= 4 && content.substr(0, 4) == "OggS";
    }

    // Contenedor MPEG-4 (usado por .m4a): box "ftyp" en el offset 4.
    inline bool matchesM4a(std::string_view content) {
        return content.size() >= 8 && content.substr(4, 4) == "ftyp";
    }

    inline bool matchesMp3(std::string_view content) {
        if (content.size() >= 3 && content.substr(0, 3) == "ID3") {
            return true;  // MP3 con tag ID3v2 al inicio
        }
        if (content.size() >= 2) {
            // Frame sync de un frame MPEG audio crudo: 11 bits en 1.
            auto first_byte  = static_cast<unsigned char>(content[0]);
            auto second_byte = static_cast<unsigned char>(content[1]);
            return first_byte == 0xFF && (second_byte & 0xE0) == 0xE0;
        }
        return false;
    }

    inline bool isValid(std::string_view contentType, std::string_view content) {
        if (contentType == Config::MIME_WAV) return matchesWav(content);
        if (contentType == Config::MIME_OGG) return matchesOgg(content);
        if (contentType == Config::MIME_M4A) return matchesM4a(content);
        if (contentType == Config::MIME_MP3) return matchesMp3(content);
        return false;
    }

}  // namespace AudioSignature

#endif  // AUDIO_SIGNATURE_H

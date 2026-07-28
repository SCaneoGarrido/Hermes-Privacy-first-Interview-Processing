#pragma once

#include <string>

namespace hermes::audio {

// Convierte cualquiera de los formatos que Sprint 3 acepta en la subida
// (wav/ogg/m4a/mp3, ver AudioSignature.h) al formato que whisper.cpp
// necesita: PCM 16-bit, 16kHz, mono. Ver ADR-014 - FFmpeg Estatico via
// vcpkg para Normalizacion de Audio.
class IAudioNormalizer {
    public:
        virtual ~IAudioNormalizer() = default;

        // Lanza std::runtime_error si el archivo no se puede leer/decodificar.
        virtual void normalize(const std::string& inputPath, const std::string& outputPath) = 0;
};

}  // namespace hermes::audio

#pragma once

#include "IAudioNormalizer.h"

namespace hermes::audio {

// Implementacion sobre libavformat/libavcodec/libswresample (FFmpeg,
// linkeado estatico via vcpkg - ver ADR-014). Decodifica y resamplea con
// las librerias de FFmpeg; el archivo de salida se escribe como WAV
// canonico a mano (sin pasar por el muxer de avformat) para no depender
// de features de encoding que este proyecto no necesita.
//
// Sin estado propio (no hay contexto compartido entre llamadas, a
// diferencia de WhisperTranscriber): no necesita mutex.
class FfmpegAudioNormalizer : public IAudioNormalizer {
    public:
        void normalize(const std::string& inputPath, const std::string& outputPath) override;
};

}  // namespace hermes::audio

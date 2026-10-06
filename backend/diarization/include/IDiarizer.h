#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace hermes::diarization {

// Tramo de habla de un hablante, en ms del audio original. speakerId es un
// numero de cluster (0, 1, ...) sin significado propio: que cluster es el
// investigador lo decide SpeakerAssigner.
struct SpeakerTurn {
    int64_t startMs;
    int64_t endMs;
    int speakerId;
};

// Diarizacion acustica ("quien habla cuando") detras de una interfaz (ADR-004,
// ADR-021): el pipeline no conoce sherpa-onnx ni onnxruntime.
class IDiarizer {
    public:
        virtual ~IDiarizer() = default;

        // false si faltan la libreria o los modelos: el pipeline sigue sin
        // identificar hablantes en vez de fallar (mismo criterio que el VAD).
        virtual bool isAvailable() = 0;

        // audioPath: WAV ya normalizado (PCM 16-bit, 16kHz, mono - ver
        // IAudioNormalizer), el mismo que recibe ITranscriber. Devuelve los
        // tramos ordenados por inicio. Lanza std::runtime_error si falla.
        virtual std::vector<SpeakerTurn> diarize(const std::string& audioPath) = 0;
};

}  // namespace hermes::diarization

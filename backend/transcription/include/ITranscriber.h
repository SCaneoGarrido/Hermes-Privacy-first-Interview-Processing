#pragma once

#include <string>
#include <vector>

namespace hermes::transcription {

// Segmento de transcripcion cruda, tal como lo devuelve whisper.cpp: sin
// identificar quien habla (eso lo resuelve Sprint 6 via Ollama sobre este
// mismo formato). start/end ya vienen formateados "HH:MM:SS" - ver
// whisper.cpp Architecture en la vault sobre la conversion de centisegundos.
struct TranscriptSegment {
    std::string start;
    std::string end;
    std::string text;
};

// Oculta whisper.cpp detras de esta interfaz (ADR-004, Filosofia de
// Repositorios). audioPath debe apuntar a un WAV ya normalizado
// (PCM 16-bit, 16kHz, mono - ver IAudioNormalizer); ITranscriber no
// normaliza nada por su cuenta.
class ITranscriber {
    public:
        virtual ~ITranscriber() = default;

        // Lanza std::runtime_error si el modelo no esta disponible o si
        // whisper.cpp falla al procesar el audio.
        virtual std::vector<TranscriptSegment> transcribe(const std::string& audioPath) = 0;
};

}  // namespace hermes::transcription

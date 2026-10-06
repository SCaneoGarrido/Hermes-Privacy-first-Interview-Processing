#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace hermes::transcription {

// Palabra con sus tiempos en el audio original (ms). Se usa para alinear la
// transcripcion con la diarizacion acustica cuando un segmento de whisper
// cruza un cambio de hablante (ver SpeakerAssigner).
struct TranscriptWord {
    int64_t startMs;
    int64_t endMs;
    std::string text;
};

// Segmento de transcripcion cruda, tal como lo devuelve whisper.cpp, sin
// identificar quien habla (eso lo resuelve la diarizacion, ver IDiarizer).
// Tiempos en milisegundos del audio original (whisper.cpp ya los remapea
// cuando usa VAD). `words` puede venir vacio (ej. tramos marcados como
// no transcritos tras un bucle).
struct TranscriptSegment {
    int64_t startMs;
    int64_t endMs;
    std::string text;
    std::vector<TranscriptWord> words;
};

// Opciones por entrevista para la transcripcion.
struct TranscriptionOptions {
    // Glosario del usuario (ver ADR-018): se usa como contexto inicial del
    // decodificador para que acierte siglas y terminos del dominio.
    std::vector<std::string> keywords;
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
        virtual std::vector<TranscriptSegment> transcribe(const std::string& audioPath,
                                                          const TranscriptionOptions& options) = 0;
};

}  // namespace hermes::transcription

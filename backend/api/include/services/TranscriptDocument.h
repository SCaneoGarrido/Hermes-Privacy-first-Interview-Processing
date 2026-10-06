#ifndef TRANSCRIPT_DOCUMENT_H
#define TRANSCRIPT_DOCUMENT_H

#include <optional>
#include <string>
#include <vector>

// Un bloque de lectura de la transcripcion: un turno de habla (si hay
// hablantes identificados) o un parrafo de texto corrido.
struct TranscriptBlock {
    // Clave del rol ("interviewer" / "subject", ver TranscriptModel); vacio
    // si el bloque no tiene hablante asignado. El nombre visible esta en
    // TranscriptDocument::speakers.
    std::optional<std::string> speaker;
    // Segundos del audio donde empieza / termina el bloque, si se conocen.
    std::optional<double> startSeconds;
    std::optional<double> endSeconds;
    std::string text;
};

// Rol de hablante con su nombre visible ("subject" -> "Monitor GES").
struct TranscriptSpeaker {
    std::string key;
    std::string label;
};

// Transcripcion final estructurada para leerla (y editarla) en la app
// (GET/PUT /interview/:id/transcript), en vez del .txt plano.
struct TranscriptDocument {
    // Aviso cuando la anonimizacion pedida fallo ("[AVISO HERMES] ...");
    // separado del texto para mostrarlo destacado.
    std::optional<std::string> notice;
    bool hasSpeakers = false;
    bool hasTimestamps = false;
    // De donde salen los hablantes: "diarization" (audio), "manual" (editado
    // por el usuario), "llm" (texto, entrevistas procesadas antes de ADR-022)
    // o "none".
    std::string speakerSource = "none";
    // true si es la version editada a mano (hay una version original del
    // pipeline que se puede restaurar).
    bool edited = false;
    // Siempre ambos roles, para que el editor pueda asignarlos aunque la
    // transcripcion todavia no tenga hablantes.
    std::vector<TranscriptSpeaker> speakers;
    std::vector<TranscriptBlock> blocks;
    std::optional<std::string> summary;
};

#endif // TRANSCRIPT_DOCUMENT_H

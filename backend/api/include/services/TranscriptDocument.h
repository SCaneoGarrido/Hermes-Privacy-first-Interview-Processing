#ifndef TRANSCRIPT_DOCUMENT_H
#define TRANSCRIPT_DOCUMENT_H

#include <optional>
#include <string>
#include <vector>

// Un bloque de lectura de la transcripcion: un turno de habla (si el texto
// fue etiquetado por hablante) o un parrafo de texto corrido.
struct TranscriptBlock {
    // "Investigador" / "Entrevistado"; vacio si la transcripcion no tiene
    // etiquetas de hablante.
    std::optional<std::string> speaker;
    // Segundo del audio donde empieza el bloque; solo cuando las lineas del
    // texto final se corresponden 1:1 con los segmentos de whisper.
    std::optional<double> startSeconds;
    std::string text;
};

// Transcripcion final estructurada para leerla en la app (GET
// /interview/:id/transcript), en vez del .txt plano.
struct TranscriptDocument {
    // Aviso que el pipeline antepone cuando la anonimizacion pedida fallo
    // ("[AVISO HERMES] ..."); separado del texto para mostrarlo destacado.
    std::optional<std::string> notice;
    bool hasSpeakers = false;
    bool hasTimestamps = false;
    std::vector<TranscriptBlock> blocks;
    std::optional<std::string> summary;
};

#endif // TRANSCRIPT_DOCUMENT_H

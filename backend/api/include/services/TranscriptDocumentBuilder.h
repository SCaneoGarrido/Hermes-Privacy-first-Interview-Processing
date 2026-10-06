#ifndef TRANSCRIPT_DOCUMENT_BUILDER_H
#define TRANSCRIPT_DOCUMENT_BUILDER_H

#include "TranscriptDocument.h"
#include "../../../transcript/include/TranscriptModel.h"
#include <string>

// Convierte la transcripcion en bloques de lectura. Pura: no lee archivos ni
// depende de infraestructura.
class TranscriptDocumentBuilder {
    public:
        // Desde la transcripcion estructurada (ADR-022):
        // - Con hablantes: un bloque por turno, uniendo fragmentos
        //   consecutivos del mismo hablante.
        // - Sin hablantes: parrafos de varios segmentos.
        // Cada bloque lleva su inicio/fin si se conocen. speakers/edited/summary
        // los completa el llamador.
        static TranscriptDocument build(const hermes::transcript::StructuredTranscript& transcript);

        // Entrevistas procesadas antes de ADR-022 (solo transcript_final.txt):
        // texto etiquetado por el LLM ("Investigador: ..." / "Entrevistado: ...")
        // o texto plano de whisper (una linea por segmento), sin tiempos.
        static TranscriptDocument buildLegacy(const std::string& transcriptText);
};

#endif // TRANSCRIPT_DOCUMENT_BUILDER_H

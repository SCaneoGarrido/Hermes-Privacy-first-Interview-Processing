#ifndef TRANSCRIPT_DOCUMENT_BUILDER_H
#define TRANSCRIPT_DOCUMENT_BUILDER_H

#include "TranscriptDocument.h"
#include <string>
#include <vector>

// Convierte el texto final del pipeline (transcript_final.txt) en bloques de
// lectura. Pura: no lee archivos ni depende de infraestructura, recibe el
// texto y los inicios de segmento de whisper ya cargados.
//
// - Texto etiquetado por hablante ("Investigador: ..." / "Entrevistado: ...",
//   ver TranscriptEnhancer): un bloque por turno; las lineas sin etiqueta se
//   suman al turno anterior.
// - Texto plano de whisper (una linea por segmento): parrafos de varias
//   lineas. Si la cantidad de lineas coincide con la de segmentos, cada
//   parrafo lleva el segundo donde empieza.
class TranscriptDocumentBuilder {
    public:
        static TranscriptDocument build(const std::string& transcriptText, const std::vector<double>& segmentStarts);
};

#endif // TRANSCRIPT_DOCUMENT_BUILDER_H

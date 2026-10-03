#pragma once

#include "ILLMClient.h"
#include "../../transcription/include/ITranscriber.h"

#include <cstddef>
#include <string>
#include <vector>

namespace hermes::llm {

// Un reemplazo aplicado, para auditar falsos positivos (ver
// glossary_changes.txt en InterviewProcessingJobHandler).
struct GlossaryChange {
    size_t line;           // 1-based: numero de segmento / linea de la transcripcion
    std::string original;  // como lo transcribio whisper
    std::string term;      // forma exacta del glosario del usuario
};

struct SanitizationResult {
    std::vector<hermes::transcription::TranscriptSegment> segments;
    std::vector<GlossaryChange> changes;
};

// Corrige variantes mal transcriptas de los terminos del glosario del
// usuario (ej. "SILYES" -> "SIGGES"). El LLM NO reescribe texto: solo
// propone pares (original, termino) por bloque; cada propuesta se valida
// de forma determinista y el reemplazo lo hace este codigo, por palabra
// completa y solo dentro del bloque donde se detecto. Asi no se puede
// perder contenido ni cambiar el formato (una linea por segmento), que es
// lo que fallo cuando el LLM reescribia el texto entero (ver ADR-017/018).
class GlossarySanitizer {
    public:
        explicit GlossarySanitizer(ILLMClient& client);

        // Lanza std::runtime_error si Ollama no responde; un bloque con
        // respuesta invalida se omite (queda sin sanitizar) y se loguea.
        SanitizationResult sanitize(const std::vector<hermes::transcription::TranscriptSegment>& segments,
                                    const std::vector<std::string>& glossary);

    private:
        ILLMClient& m_client;
};

}  // namespace hermes::llm

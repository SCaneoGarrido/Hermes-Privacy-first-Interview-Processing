#pragma once

#include "ILLMClient.h"
#include "../../transcription/include/ITranscriber.h"

#include <functional>
#include <string>
#include <vector>

namespace hermes::llm {

// Salida de las 3 fases de Sprint 6 (ver Ollama Integration Strategy en la
// vault). Cada campo es independiente para que el llamador pueda degradar
// con gracia: si anonymizedTranscript esta vacio (fallo la Fase 2),
// correctedTranscript sigue siendo utilizable.
struct EnhancementResult {
    std::string correctedTranscript;   // Fase 1: corregido + hablante etiquetado (best-effort)
    std::string anonymizedTranscript;  // Fase 2: PII reemplazada por placeholders
    std::string summary;               // Fase 3: resumen final (map-reduce)
    // Vacio si todas las fases pedidas terminaron. Si no, describe la fase
    // que fallo; los campos de las fases previas siguen siendo validos.
    std::string failure;
};

// Orquesta las 3 fases sobre los segmentos crudos de whisper.cpp,
// chunkeando para no exceder la ventana de contexto de un LLM local (ver
// Ollama Integration Strategy - una entrevista real de 70 min genera 1804
// segmentos, muy por encima de cualquier prompt unico razonable).
// Se invoca en cada transicion de fase con un paso corto (ej.
// "corrigiendo_texto") - pensado para que el llamador lo persista en
// interview_jobs.current_step (ver IInterviewJobRepository) sin que esta
// clase tenga que conocer DatabaseManager (Filosofia de Repositorios).
using ProgressCallback = std::function<void(const std::string& step)>;

class TranscriptEnhancer {
    public:
        explicit TranscriptEnhancer(ILLMClient& client);

        // enhanceTranscript: Fases 1-2 (correccion + anonimizacion), opt-in
        // (ver Job.h). Si es false, correctedTranscript y anonymizedTranscript
        // quedan vacios y el resumen (si se pide) se hace sobre el texto
        // plano de whisper.
        // includeSummary: Fase 3 (resumen) es opcional y a pedido explicito
        // (ver Job.h) - no forma parte de lo que el programa espera por
        // defecto, y agrega ~30% de llamadas a Ollama sobre el total. Si es
        // false, summary queda vacio y esa fase ni se ejecuta.
        // onProgress: opcional, ver ProgressCallback arriba.
        // Lanza std::runtime_error solo si falla la Fase 1 (no hay nada que
        // conservar). Si falla una fase posterior, devuelve lo ya obtenido con
        // `failure` explicando cual - el llamador decide como degradar (ver
        // InterviewProcessingJobHandler). Si se pidio anonimizar y fallo, no
        // se genera el resumen sobre el texto sin anonimizar.
        EnhancementResult enhance(const std::vector<hermes::transcription::TranscriptSegment>& segments,
                                   bool enhanceTranscript,
                                   bool includeSummary,
                                   const ProgressCallback& onProgress = {});

    private:
        ILLMClient& m_client;

        std::string correctAndStructure(const std::vector<hermes::transcription::TranscriptSegment>& segments);
        std::string anonymize(const std::string& correctedText);
        std::string summarize(const std::string& anonymizedText);
};

}  // namespace hermes::llm

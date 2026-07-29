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

        // includeSummary: Fase 3 (resumen) es opcional y a pedido explicito
        // (ver Job.h) - no forma parte de lo que el programa espera por
        // defecto, y agrega ~30% de llamadas a Ollama sobre el total. Si es
        // false, summary queda vacio y esa fase ni se ejecuta.
        // onProgress: opcional, ver ProgressCallback arriba.
        // Lanza std::runtime_error si Ollama falla en cualquier fase - el
        // llamador decide como degradar (ver InterviewProcessingJobHandler).
        EnhancementResult enhance(const std::vector<hermes::transcription::TranscriptSegment>& segments,
                                   bool includeSummary,
                                   const ProgressCallback& onProgress = {});

    private:
        ILLMClient& m_client;

        std::string correctAndStructure(const std::vector<hermes::transcription::TranscriptSegment>& segments);
        std::string anonymize(const std::string& correctedText);
        std::string summarize(const std::string& anonymizedText);
};

}  // namespace hermes::llm

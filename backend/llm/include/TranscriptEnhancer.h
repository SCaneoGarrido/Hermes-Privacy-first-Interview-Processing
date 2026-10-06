#pragma once

#include "ILLMClient.h"
#include "../../transcript/include/TranscriptModel.h"

#include <functional>
#include <string>
#include <vector>

namespace hermes::llm {

// Resultado de las fases opcionales via Ollama (ver Ollama Integration
// Strategy en la vault). Los textos corregidos/anonimizados se aplican sobre
// los turnos de la transcripcion recibida (sin tocar hablantes ni tiempos);
// esto solo informa que paso, para que el llamador pueda degradar con gracia.
struct EnhancementResult {
    bool corrected = false;             // Fase 1 aplicada a los turnos
    bool anonymized = false;            // Fase 2 aplicada a los turnos
    std::string correctedTranscript;    // render de la Fase 1 (trazabilidad, transcript_corrected.txt)
    std::string summary;                // Fase 3: resumen final (map-reduce)
    // Vacio si todas las fases pedidas terminaron. Si no, describe la fase
    // que fallo; lo aplicado por las fases previas sigue siendo valido.
    std::string failure;
};

// Se invoca en cada transicion de fase con un paso corto (ej.
// "corrigiendo_texto") - pensado para que el llamador lo persista en
// interview_jobs.current_step (ver IInterviewJobRepository) sin que esta
// clase tenga que conocer DatabaseManager (Filosofia de Repositorios).
using ProgressCallback = std::function<void(const std::string& step)>;

// Orquesta las fases opcionales sobre la transcripcion estructurada,
// chunkeando para no exceder la ventana de contexto de un LLM local (una
// entrevista real de 70 min genera ~1800 segmentos).
//
// Desde ADR-022 el LLM NO atribuye hablantes (eso lo hace la diarizacion
// acustica, ver IDiarizer): la correccion trabaja turno por turno, por
// indice, y si el modelo omite o rompe una linea se conserva el texto
// original - asi no se pierde contenido entre bloques (ADR-017).
class TranscriptEnhancer {
    public:
        explicit TranscriptEnhancer(ILLMClient& client);

        // enhanceTranscript: Fases 1-2 (correccion + anonimizacion), opt-in
        // (ver Job.h). includeSummary: Fase 3, opt-in.
        // labels: nombres de los roles, para el contexto de la correccion y el
        // texto que se resume.
        // Lanza std::runtime_error solo si falla la Fase 1 (transcript queda
        // intacta). Si falla una fase posterior, devuelve lo ya obtenido con
        // `failure` explicando cual. Si se pidio anonimizar y fallo, no se
        // genera el resumen sobre el texto sin anonimizar.
        EnhancementResult enhance(hermes::transcript::StructuredTranscript& transcript,
                                   const hermes::transcript::SpeakerLabels& labels,
                                   bool enhanceTranscript,
                                   bool includeSummary,
                                   const ProgressCallback& onProgress = {});

    private:
        ILLMClient& m_client;

        // Devuelve los textos corregidos, uno por turno (mismo orden y tamaño).
        std::vector<std::string> correctTurns(const hermes::transcript::StructuredTranscript& transcript,
                                              const hermes::transcript::SpeakerLabels& labels);
        // Devuelve los textos anonimizados, uno por texto recibido.
        std::vector<std::string> anonymize(const std::vector<std::string>& texts);
        std::string summarize(const std::string& text);
};

}  // namespace hermes::llm

#pragma once

#include <chrono>
#include <string>

namespace hermes::jobs {

// Struct de valor: lo que viaja por la cola en memoria. No es la fila de
// interview_jobs (eso lo modela InterviewJobRecord) - Job es solo lo minimo
// que un worker necesita para saber que ejecutar.
struct Job {
    int interviewId;
    // Unico tipo por ahora ("interview_processing"); existe para no romper
    // el contrato de IJobHandler cuando Sprint 5/6 agreguen mas tipos.
    std::string type = "interview_processing";
    std::chrono::system_clock::time_point createdAt = std::chrono::system_clock::now();
    // Fase 3 de Sprint 6 (resumen) es opcional, a pedido explicito del
    // usuario via POST /interview/:id/process - no es parte de lo que el
    // programa espera por defecto (solo transcripcion + anonimizacion lo
    // son), y agrega ~30% de llamadas a Ollama sobre el total. Default
    // false: el researcher lo prende cuando lo quiere.
    bool includeSummary = false;
    // Correccion + etiquetado de hablante + anonimizacion via Ollama (Fases
    // 1-2 de Sprint 6). Opt-in desde ADR-017: en la validacion manual de la
    // entrevista 13 la anonimizacion reemplazo sustantivos comunes y la
    // correccion perdio texto, y el investigador ya excluye la PII antes de
    // grabar. Default false: la salida es la transcripcion plana de whisper.
    bool enhanceTranscript = false;
};

}  // namespace hermes::jobs

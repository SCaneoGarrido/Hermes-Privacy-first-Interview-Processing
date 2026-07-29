#include "../include/InterviewProcessingJobHandler.h"
#include "../../api/include/logger.h"
#include "../../api/include/Constanst.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace hermes::jobs {

namespace {

// Trim simple: whisper.cpp suele devolver los segmentos con un espacio
// inicial (" Hola, gracias...") - ver el ejemplo en whisper.cpp Architecture.
std::string trim(const std::string& text) {
    const size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    const size_t last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

void writeRawTranscriptJson(const std::vector<hermes::transcription::TranscriptSegment>& segments, const std::string& path) {
    nlohmann::json json = nlohmann::json::array();
    for (const auto& segment : segments) {
        json.push_back({
            {"start", segment.start},
            {"end", segment.end},
            {"text", segment.text},
        });
    }

    std::ofstream out(path);
    if (!out.is_open()) {
        throw std::runtime_error("No se pudo crear el archivo de transcripcion cruda: " + path);
    }
    // error_handler_t::replace: segunda red de seguridad ademas del
    // sanitizeUtf8 en WhisperTranscriber - si algo igual llega con UTF-8
    // invalido, se reemplaza por U+FFFD en vez de tirar type_error.316 y
    // perder el job entero (visto en produccion con audio real largo).
    out << json.dump(2, ' ', false, nlohmann::json::error_handler_t::replace);
}

void writePlainTranscript(const std::vector<hermes::transcription::TranscriptSegment>& segments, const std::string& path) {
    std::ofstream out(path);
    if (!out.is_open()) {
        throw std::runtime_error("No se pudo crear el archivo de transcripcion final: " + path);
    }
    for (const auto& segment : segments) {
        out << trim(segment.text) << "\n";
    }
}

}  // namespace

InterviewProcessingJobHandler::InterviewProcessingJobHandler(IInterviewRepository& interviewRepository,
                                                               IInterviewJobRepository& jobRepository,
                                                               hermes::audio::IAudioNormalizer& audioNormalizer,
                                                               hermes::transcription::ITranscriber& transcriber,
                                                               hermes::llm::TranscriptEnhancer& transcriptEnhancer)
    : m_interviewRepository(interviewRepository),
      m_jobRepository(jobRepository),
      m_audioNormalizer(audioNormalizer),
      m_transcriber(transcriber),
      m_transcriptEnhancer(transcriptEnhancer) {}

void InterviewProcessingJobHandler::execute(const Job& job) {
    auto audio = m_interviewRepository.findAudioByInterviewId(job.interviewId);
    if (!audio.has_value()) {
        // No deberia pasar: InterviewService.requestProcessing ya valida
        // que haya audio antes de encolar. Se trata igual como fallo del
        // job (interview_jobs.status = failed) en vez de crashear el worker.
        throw std::runtime_error("La entrevista no tiene audio asociado (interview_id=" + std::to_string(job.interviewId) + ")");
    }

    const std::filesystem::path interviewDir =
        std::filesystem::path(std::string(Config::STORAGE_DIRECTORY)) / "interviews" / std::to_string(job.interviewId);
    std::filesystem::create_directories(interviewDir);

    log_event("[InterviewProcessingJobHandler][execute] Normalizando audio, interview_id=" + std::to_string(job.interviewId));
    m_jobRepository.updateCurrentStep(job.interviewId, "normalizando_audio");
    const std::string normalizedPath = (interviewDir / "audio.wav").string();
    m_audioNormalizer.normalize(audio->path, normalizedPath);

    log_event("[InterviewProcessingJobHandler][execute] Transcribiendo con whisper.cpp, interview_id=" + std::to_string(job.interviewId));
    m_jobRepository.updateCurrentStep(job.interviewId, "transcribiendo");
    auto segments = m_transcriber.transcribe(normalizedPath);

    const std::string rawJsonPath = (interviewDir / "transcript_raw.json").string();
    writeRawTranscriptJson(segments, rawJsonPath);
    if (!m_jobRepository.saveRawTranscriptPath(job.interviewId, rawJsonPath)) {
        log_event("[InterviewProcessingJobHandler][execute] No se pudo guardar raw_transcript_path, interview_id=" + std::to_string(job.interviewId));
    }

    // Linea de base (Sprint 5): transcripcion cruda concatenada, sin
    // diarizar. Se guarda ANTES de intentar Ollama a proposito - si Ollama
    // falla mas abajo, esto sigue siendo el resultado disponible.
    const std::string finalTxtPath = (interviewDir / "transcript_final.txt").string();
    writePlainTranscript(segments, finalTxtPath);
    if (!m_interviewRepository.upsertTranscriptionResult(job.interviewId, finalTxtPath)) {
        throw std::runtime_error("No se pudo guardar el resultado de transcripcion en interview_results (interview_id=" + std::to_string(job.interviewId) + ")");
    }

    // Sprint 6: correccion+estructuracion por hablante, anonimizacion y
    // resumen via Ollama. Degradacion con gracia: si Ollama no esta
    // corriendo, no tiene el modelo, o falla en cualquier fase, el
    // resultado de Sprint 5 de arriba sigue siendo el disponible - no se
    // relanza la excepcion, no se tumba el job completo por esto.
    try {
        log_event("[InterviewProcessingJobHandler][execute] Mejorando transcripcion con Ollama (resumen=" +
                   std::string(job.includeSummary ? "si" : "no") + "), interview_id=" + std::to_string(job.interviewId));
        auto enhancement = m_transcriptEnhancer.enhance(segments, job.includeSummary,
            [this, &job](const std::string& step) {
                m_jobRepository.updateCurrentStep(job.interviewId, step);
            });

        std::ofstream finalOut(finalTxtPath);
        if (!finalOut.is_open()) {
            throw std::runtime_error("No se pudo reescribir " + finalTxtPath);
        }
        finalOut << enhancement.anonymizedTranscript;
        finalOut.close();
        if (!m_interviewRepository.upsertTranscriptionResult(job.interviewId, finalTxtPath)) {
            throw std::runtime_error("No se pudo actualizar interview_results con el resultado de Ollama");
        }

        // El resumen es opcional (ver Job.h) - si no se pidio, ni se genera
        // el archivo ni se toca summary_file_path (queda NULL).
        if (job.includeSummary) {
            const std::string summaryPath = (interviewDir / "transcript_summary.txt").string();
            std::ofstream summaryOut(summaryPath);
            if (!summaryOut.is_open()) {
                throw std::runtime_error("No se pudo crear " + summaryPath);
            }
            summaryOut << enhancement.summary;
            summaryOut.close();
            if (!m_interviewRepository.updateSummaryPath(job.interviewId, summaryPath)) {
                throw std::runtime_error("No se pudo guardar summary_file_path");
            }
        }

        log_event("[InterviewProcessingJobHandler][execute] Ollama OK, interview_id=" + std::to_string(job.interviewId));
    } catch (const std::exception& e) {
        log_event("[InterviewProcessingJobHandler][execute] Ollama fallo, se mantiene la transcripcion sin diarizar de Sprint 5. interview_id=" +
                   std::to_string(job.interviewId) + ". Detalle: " + e.what());
    }
}

}  // namespace hermes::jobs

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

// Encabezado de la transcripcion entregada cuando la anonimizacion no se
// aplico: el archivo contiene datos personales y quien lo descargue tiene
// que saberlo antes de compartirlo (privacidad, ver CLAUDE.md).
std::string nonAnonymizedNotice(const std::string& reason) {
    return "[AVISO HERMES] Esta transcripcion NO fue anonimizada y puede contener datos personales. "
           "Revisala antes de compartirla. Motivo: " + reason + "\n\n";
}

void writeTextFile(const std::string& path, const std::string& content) {
    std::ofstream out(path);
    if (!out.is_open()) {
        throw std::runtime_error("No se pudo escribir " + path);
    }
    out << content;
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
                                                               hermes::llm::GlossarySanitizer& glossarySanitizer,
                                                               hermes::llm::TranscriptEnhancer& transcriptEnhancer)
    : m_interviewRepository(interviewRepository),
      m_jobRepository(jobRepository),
      m_audioNormalizer(audioNormalizer),
      m_transcriber(transcriber),
      m_glossarySanitizer(glossarySanitizer),
      m_transcriptEnhancer(transcriptEnhancer) {}

void InterviewProcessingJobHandler::replaceVideoWithExtractedAudio(int interviewId,
                                                                    const InterviewAudioRecord& video,
                                                                    const std::string& normalizedPath) {
    // Copia (no mueve) audio.wav junto al upload original: si la entrevista
    // se reprocesa, el normalizador lee de uploads/ y escribe audio.wav, sin
    // leer y escribir el mismo archivo. Orden copia -> BD -> borrado: la BD
    // nunca apunta a un archivo inexistente. Ningun fallo aca aborta el job -
    // el audio ya esta normalizado; en el peor caso el video queda en disco.
    const std::string idTag = " (interview_id=" + std::to_string(interviewId) + ")";
    std::filesystem::path extractedPath(video.path);
    extractedPath.replace_extension(".wav");

    std::error_code ec;
    std::filesystem::copy_file(normalizedPath, extractedPath, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        log_event("[InterviewProcessingJobHandler][replaceVideoWithExtractedAudio] No se pudo copiar el audio extraido: " + ec.message() + idTag);
        return;
    }

    const auto extractedSize = static_cast<long long>(std::filesystem::file_size(extractedPath, ec));
    if (ec || !m_interviewRepository.updateAudio(interviewId, extractedPath.string(), ".wav", extractedSize)) {
        log_event("[InterviewProcessingJobHandler][replaceVideoWithExtractedAudio] No se pudo registrar el audio extraido, se conserva el video" + idTag);
        std::filesystem::remove(extractedPath, ec);
        return;
    }

    std::filesystem::remove(video.path, ec);
    if (ec) {
        log_event("[InterviewProcessingJobHandler][replaceVideoWithExtractedAudio] No se pudo borrar el video original " + video.path + ": " + ec.message() + idTag);
        return;
    }
    log_event("[InterviewProcessingJobHandler][replaceVideoWithExtractedAudio] Video original reemplazado por su audio extraido" + idTag);
}

void InterviewProcessingJobHandler::applyGlossary(int interviewId,
                                                  const std::vector<std::string>& keywords,
                                                  const std::filesystem::path& interviewDir,
                                                  std::vector<hermes::transcription::TranscriptSegment>& segments) {
    const std::string idTag = " (interview_id=" + std::to_string(interviewId) + ")";
    log_event("[InterviewProcessingJobHandler][applyGlossary] Aplicando " + std::to_string(keywords.size()) + " palabras clave" + idTag);
    m_jobRepository.updateCurrentStep(interviewId, "aplicando_glosario");
    try {
        auto sanitized = m_glossarySanitizer.sanitize(segments, keywords);
        segments = std::move(sanitized.segments);

        // Para auditar falsos positivos: que se cambio y en que linea.
        std::string report;
        for (const auto& change : sanitized.changes) {
            report += "linea " + std::to_string(change.line) + ": " + change.original + " -> " + change.term + "\n";
        }
        writeTextFile((interviewDir / "glossary_changes.txt").string(), report);
        log_event("[InterviewProcessingJobHandler][applyGlossary] " + std::to_string(sanitized.changes.size()) +
                  " reemplazos aplicados" + idTag);
    } catch (const std::exception& e) {
        log_event("[InterviewProcessingJobHandler][applyGlossary] No se pudo aplicar el glosario, se mantiene la transcripcion de whisper" +
                  idTag + ". Detalle: " + e.what());
    }
}

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
    if (Config::isVideoExtension(audio->format)) {
        replaceVideoWithExtractedAudio(job.interviewId, *audio, normalizedPath);
    }

    log_event("[InterviewProcessingJobHandler][execute] Transcribiendo con whisper.cpp, interview_id=" + std::to_string(job.interviewId));
    m_jobRepository.updateCurrentStep(job.interviewId, "transcribiendo");
    hermes::transcription::TranscriptionOptions transcriptionOptions;
    transcriptionOptions.keywords = m_interviewRepository.findKeywordsByInterviewId(job.interviewId);
    auto segments = m_transcriber.transcribe(normalizedPath, transcriptionOptions);

    const std::string rawJsonPath = (interviewDir / "transcript_raw.json").string();
    writeRawTranscriptJson(segments, rawJsonPath);
    if (!m_jobRepository.saveRawTranscriptPath(job.interviewId, rawJsonPath)) {
        log_event("[InterviewProcessingJobHandler][execute] No se pudo guardar raw_transcript_path, interview_id=" + std::to_string(job.interviewId));
    }

    // transcript_raw.json (arriba) queda tal cual lo dio whisper; la
    // transcripcion entregada lleva el glosario aplicado.
    if (!transcriptionOptions.keywords.empty()) {
        applyGlossary(job.interviewId, transcriptionOptions.keywords, interviewDir, segments);
    }

    // Linea de base (Sprint 5): transcripcion cruda concatenada, sin
    // diarizar. Se guarda ANTES de intentar Ollama a proposito - si Ollama
    // falla mas abajo, esto sigue siendo el resultado disponible.
    const std::string finalTxtPath = (interviewDir / "transcript_final.txt").string();
    writePlainTranscript(segments, finalTxtPath);
    if (!m_interviewRepository.upsertTranscriptionResult(job.interviewId, finalTxtPath)) {
        throw std::runtime_error("No se pudo guardar el resultado de transcripcion en interview_results (interview_id=" + std::to_string(job.interviewId) + ")");
    }

    // Fases opcionales via Ollama (ver Job.h): sin ninguna pedida, la
    // transcripcion plana de arriba es el resultado final y no se llama a
    // Ollama en absoluto.
    if (!job.enhanceTranscript && !job.includeSummary) {
        return;
    }

    // Degradacion con gracia: si Ollama no esta corriendo, no tiene el
    // modelo, o falla en cualquier fase, se conserva lo ya obtenido - no se
    // relanza la excepcion, no se tumba el job completo por esto.
    try {
        log_event("[InterviewProcessingJobHandler][execute] Ollama (correccion=" +
                   std::string(job.enhanceTranscript ? "si" : "no") + ", resumen=" +
                   std::string(job.includeSummary ? "si" : "no") + "), interview_id=" + std::to_string(job.interviewId));
        auto enhancement = m_transcriptEnhancer.enhance(segments, job.enhanceTranscript, job.includeSummary,
            [this, &job](const std::string& step) {
                m_jobRepository.updateCurrentStep(job.interviewId, step);
            });

        if (job.enhanceTranscript) {
            // Resultado intermedio de la Fase 1, conservado aunque fallen las
            // siguientes (antes una falla en la anonimizacion lo descartaba).
            writeTextFile((interviewDir / "transcript_corrected.txt").string(), enhancement.correctedTranscript);

            // Se pidio anonimizar: si fallo, el archivo entregado lo avisa.
            if (enhancement.anonymizedTranscript.empty()) {
                writeTextFile(finalTxtPath, nonAnonymizedNotice(enhancement.failure) + enhancement.correctedTranscript);
            } else {
                writeTextFile(finalTxtPath, enhancement.anonymizedTranscript);
            }
            if (!m_interviewRepository.upsertTranscriptionResult(job.interviewId, finalTxtPath)) {
                throw std::runtime_error("No se pudo actualizar interview_results con el resultado de Ollama");
            }
        }

        // El resumen es siempre un documento aparte de la transcripcion
        // (GET .../summary). Si no se pidio o su fase no llego a correr, ni
        // se genera el archivo ni se toca summary_file_path.
        if (job.includeSummary && !enhancement.summary.empty()) {
            const std::string summaryPath = (interviewDir / "transcript_summary.txt").string();
            writeTextFile(summaryPath, enhancement.summary);
            if (!m_interviewRepository.updateSummaryPath(job.interviewId, summaryPath)) {
                throw std::runtime_error("No se pudo guardar summary_file_path");
            }
        }

        if (enhancement.failure.empty()) {
            log_event("[InterviewProcessingJobHandler][execute] Ollama OK, interview_id=" + std::to_string(job.interviewId));
        } else {
            log_event("[InterviewProcessingJobHandler][execute] Ollama completo parcialmente, interview_id=" +
                       std::to_string(job.interviewId) + ". Fallo en " + enhancement.failure);
        }
    } catch (const std::exception& e) {
        log_event("[InterviewProcessingJobHandler][execute] Ollama fallo, se mantiene la transcripcion de whisper. interview_id=" +
                   std::to_string(job.interviewId) + ". Detalle: " + e.what());
        // Solo si se pidio anonimizar el archivo entregado tiene que avisar
        // que no lo esta; si solo fallo el resumen, la transcripcion queda intacta.
        if (job.enhanceTranscript) {
            std::string plain;
            for (const auto& segment : segments) {
                plain += trim(segment.text) + "\n";
            }
            writeTextFile(finalTxtPath, nonAnonymizedNotice(std::string("correccion: ") + e.what()) + plain);
        }
    }
}

}  // namespace hermes::jobs

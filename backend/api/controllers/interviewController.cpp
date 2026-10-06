#include "crow.h"
#include "../include/interviewController.h"
#include "../include/InterviewControllerHelper.h"
#include "../include/ApiResponse.h"
#include "../include/logger.h"
#include <fstream>
#include <optional>
#include <sstream>
#include <vector>

namespace {

// Arma la respuesta de descarga de un texto (transcripcion/resumen). No pasa
// por ApiResponse: el body es el texto crudo, no el sobre {success,data,error}
// - los casos de error (entrevista no encontrada, archivo no listo) si usan
// ApiResponse, para mantener el contrato JSON en esos casos.
crow::response buildTextDownload(const std::string& content, const std::string& downloadFilename) {
    // BOM UTF-8 al inicio de la respuesta (no del archivo en disco): sin
    // el, editores de texto en Windows sin deteccion de codificacion
    // confiable (Notepad clasico, entre otros) adivinan la pagina de
    // codigos ANSI del sistema en vez de UTF-8, y los acentos se ven como
    // mojibake pese a que el archivo es UTF-8 valido.
    static const std::string UTF8_BOM = "\xEF\xBB\xBF";

    crow::response res(200, UTF8_BOM + content);
    res.set_header("Content-Type", "text/plain; charset=utf-8");
    res.set_header("Content-Disposition", "attachment; filename=\"" + downloadFilename + "\"");
    return res;
}

crow::response buildDownloadResponse(const std::string& path, const std::string& downloadFilename) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        log_event("[interviewController][buildDownloadResponse] No se pudo abrir el archivo: " + path);
        return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "No se pudo leer el archivo solicitado");
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buildTextDownload(buffer.str(), downloadFilename);
}

crow::json::wvalue serializeTranscriptDocument(int id, const TranscriptDocument& document) {
    crow::json::wvalue data;
    data["id"] = id;
    data["has_speakers"] = document.hasSpeakers;
    data["has_timestamps"] = document.hasTimestamps;
    data["speaker_source"] = document.speakerSource;
    data["edited"] = document.edited;
    if (document.notice) {
        data["notice"] = *document.notice;
    } else {
        data["notice"] = nullptr;
    }
    if (document.summary) {
        data["summary"] = *document.summary;
    } else {
        data["summary"] = nullptr;
    }

    std::vector<crow::json::wvalue> speakers;
    for (const auto& speaker : document.speakers) {
        crow::json::wvalue item;
        item["key"] = speaker.key;
        item["label"] = speaker.label;
        speakers.push_back(std::move(item));
    }
    data["speakers"] = std::move(speakers);

    std::vector<crow::json::wvalue> blocks;
    blocks.reserve(document.blocks.size());
    for (const auto& block : document.blocks) {
        crow::json::wvalue item;
        if (block.speaker) {
            item["speaker"] = *block.speaker;
        } else {
            item["speaker"] = nullptr;
        }
        if (block.startSeconds) {
            item["start"] = *block.startSeconds;
        } else {
            item["start"] = nullptr;
        }
        if (block.endSeconds) {
            item["end"] = *block.endSeconds;
        } else {
            item["end"] = nullptr;
        }
        item["text"] = block.text;
        blocks.push_back(std::move(item));
    }
    data["blocks"] = std::move(blocks);
    return data;
}

crow::response transcriptEditResponse(int id, const TranscriptEditOutcome& outcome) {
    using Status = TranscriptEditOutcome::Status;
    switch (outcome.status) {
        case Status::NotFound:
            return ApiResponse::failure(404, "NOT_FOUND", "No existe una entrevista con ese id");
        case Status::NotReady:
            return ApiResponse::failure(409, "TRANSCRIPTION_NOT_READY", "La entrevista todavia no tiene una transcripcion disponible");
        case Status::NotEditable:
            return ApiResponse::failure(409, "TRANSCRIPTION_NOT_EDITABLE",
                                        "Esta transcripcion se genero con una version anterior de Hermes. Volvé a procesar la entrevista para poder editarla.");
        case Status::Busy:
            return ApiResponse::failure(409, "INTERVIEW_BUSY", "La entrevista se esta procesando; esperá a que termine para editar la transcripcion");
        case Status::Invalid:
            return ApiResponse::failure(400, "VALIDATION_ERROR", outcome.message);
        case Status::Failed:
            return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "No se pudo guardar la transcripcion");
        case Status::Ok:
            break;
    }
    return ApiResponse::success(200, serializeTranscriptDocument(id, outcome.document));
}

std::optional<double> readOptionalNumber(const crow::json::rvalue& item, const char* key) {
    if (!item.has(key)) return std::nullopt;
    const auto& value = item[key];
    if (value.t() != crow::json::type::Number) return std::nullopt;
    return value.d();
}

}  // namespace

InterviewController::InterviewController(InterviewService& service) : m_service(service) {}

crow::response InterviewController::handleInterviewRegistration(const crow::request& req) {
    try {
        crow::json::rvalue body_json = crow::json::load(req.body);
        auto [date, type, subject_type] = getFields(body_json);

        auto id = m_service.createInterview(date, type, subject_type);
        if (id.has_value()) {
            crow::json::wvalue data;
            data["code"] = "CREATED";
            data["id"] = id.value();
            return ApiResponse::success(201, std::move(data));
        }

        log_event("[interviewController][handleInterviewRegistration] Fallo en handleInterviewRegistration. Error al almacenar la informacion de entrevista");
        return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error registrando entrevista");

    } catch (std::exception& e) {
        std::stringstream log_error_ss;
        log_error_ss << "[interviewController][handleInterviewRegistration] Fallo en handleInterviewRegistration. Detalle: " << e.what();
        log_event(log_error_ss.str());
        return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error registrando entrevista");
    }
}

crow::response InterviewController::getInterviews(const crow::request& req) {
    try {
        auto interviews = m_service.listInterviews();

        std::vector<crow::json::wvalue> data;
        data.reserve(interviews.size());

        for (const auto& interview : interviews) {
            crow::json::wvalue item;
            item["id"] = interview.id;
            item["date"] = interview.date;
            item["type"] = interview.type;
            item["subject_type"] = interview.subjectType;
            item["status"] = interview.status;
            item["created_at"] = interview.createdAt;
            data.push_back(std::move(item));
        }

        crow::json::wvalue body;
        body = std::move(data);
        return ApiResponse::success(200, std::move(body));

    } catch (const std::exception& e) {
        std::stringstream log_error_ss;
        log_error_ss << "[interviewController][getInterviews] Fallo en getInterviews. Detalle: " << e.what();
        log_event(log_error_ss.str());
        return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error obteniendo el listado de entrevistas");
    }
}

crow::response InterviewController::getInterview(int id) {
    try {
        auto detail = m_service.getInterviewDetail(id);
        if (!detail.has_value()) {
            return ApiResponse::failure(404, "NOT_FOUND", "No existe una entrevista con ese id");
        }

        const auto& interview = detail->interview;
        crow::json::wvalue data;
        data["id"] = interview.id;
        data["date"] = interview.date;
        data["type"] = interview.type;
        data["subject_type"] = interview.subjectType;
        data["status"] = interview.status;
        data["created_at"] = interview.createdAt;

        if (detail->audio.has_value()) {
            crow::json::wvalue audio;
            audio["path"] = detail->audio->path;
            audio["format"] = detail->audio->format;
            audio["size"] = detail->audio->size;
            data["audio"] = std::move(audio);
        } else {
            data["audio"] = nullptr;
        }

        if (detail->transcriptionPath.has_value()) {
            crow::json::wvalue result;
            result["transcription_file_path"] = detail->transcriptionPath.value();
            if (detail->summaryPath.has_value()) {
                result["summary_file_path"] = detail->summaryPath.value();
            } else {
                result["summary_file_path"] = nullptr;
            }
            data["result"] = std::move(result);
        } else {
            data["result"] = nullptr;
        }

        if (detail->executionTimeSeconds.has_value()) {
            data["execution_time_seconds"] = detail->executionTimeSeconds.value();
        } else {
            data["execution_time_seconds"] = nullptr;
        }

        if (detail->currentStep.has_value()) {
            data["current_step"] = detail->currentStep.value();
        } else {
            data["current_step"] = nullptr;
        }

        std::vector<crow::json::wvalue> keywords;
        for (const auto& keyword : detail->keywords) {
            keywords.emplace_back(keyword);
        }
        data["keywords"] = std::move(keywords);
        data["transcript_edited"] = detail->transcriptEdited;

        return ApiResponse::success(200, std::move(data));

    } catch (const std::exception& e) {
        std::stringstream log_error_ss;
        log_error_ss << "[interviewController][getInterview] Fallo en getInterview. Detalle: " << e.what();
        log_event(log_error_ss.str());
        return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error obteniendo la entrevista");
    }
}

crow::response InterviewController::downloadTranscript(int id) {
    try {
        using Status = TranscriptTextOutcome::Status;
        auto outcome = m_service.renderTranscriptText(id);
        switch (outcome.status) {
            case Status::NotFound:
                return ApiResponse::failure(404, "NOT_FOUND", "No existe una entrevista con ese id");
            case Status::NotReady:
                return ApiResponse::failure(409, "TRANSCRIPTION_NOT_READY", "La entrevista todavia no tiene una transcripcion disponible");
            case Status::FileMissing:
                return ApiResponse::failure(410, "TRANSCRIPTION_FILE_MISSING",
                                            "El archivo de la transcripción ya no está en disco. Volvé a procesar la entrevista.");
            case Status::Ok:
                break;
        }
        return buildTextDownload(outcome.text, "entrevista_" + std::to_string(id) + "_transcripcion.txt");

    } catch (const std::exception& e) {
        std::stringstream log_error_ss;
        log_error_ss << "[interviewController][downloadTranscript] Fallo en downloadTranscript. Detalle: " << e.what();
        log_event(log_error_ss.str());
        return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error descargando la transcripcion");
    }
}

crow::response InterviewController::downloadSummary(int id) {
    try {
        auto detail = m_service.getInterviewDetail(id);
        if (!detail.has_value()) {
            return ApiResponse::failure(404, "NOT_FOUND", "No existe una entrevista con ese id");
        }
        if (!detail->summaryPath.has_value()) {
            return ApiResponse::failure(409, "SUMMARY_NOT_READY", "La entrevista no tiene un resumen disponible (no se pidio al procesar, o Ollama fallo)");
        }

        return buildDownloadResponse(detail->summaryPath.value(), "entrevista_" + std::to_string(id) + "_resumen.txt");

    } catch (const std::exception& e) {
        std::stringstream log_error_ss;
        log_error_ss << "[interviewController][downloadSummary] Fallo en downloadSummary. Detalle: " << e.what();
        log_event(log_error_ss.str());
        return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error descargando el resumen");
    }
}

crow::response InterviewController::getTranscript(int id) {
    try {
        using Status = TranscriptDocumentOutcome::Status;
        auto outcome = m_service.getTranscriptDocument(id);
        switch (outcome.status) {
            case Status::NotFound:
                return ApiResponse::failure(404, "NOT_FOUND", "No existe una entrevista con ese id");
            case Status::NotReady:
                return ApiResponse::failure(409, "TRANSCRIPTION_NOT_READY", "La entrevista todavia no tiene una transcripcion disponible");
            case Status::FileMissing:
                return ApiResponse::failure(410, "TRANSCRIPTION_FILE_MISSING",
                                            "El archivo de la transcripción ya no está en disco. Volvé a procesar la entrevista.");
            case Status::Ok:
                break;
        }

        return ApiResponse::success(200, serializeTranscriptDocument(id, outcome.document));

    } catch (const std::exception& e) {
        std::stringstream log_error_ss;
        log_error_ss << "[interviewController][getTranscript] Fallo en getTranscript. Detalle: " << e.what();
        log_event(log_error_ss.str());
        return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error leyendo la transcripcion");
    }
}

crow::response InterviewController::updateTranscript(const crow::request& req, int id) {
    try {
        // Body: {"blocks": [{"speaker": "interviewer"|"subject"|null,
        //                    "start": 12.3|null, "end": 20.1|null, "text": "..."}]}
        crow::json::rvalue body = crow::json::load(req.body);
        if (!body || !body.has("blocks") || body["blocks"].t() != crow::json::type::List) {
            return ApiResponse::failure(400, "VALIDATION_ERROR", "Se esperaba un objeto JSON con la lista 'blocks'");
        }

        std::vector<TranscriptBlockInput> blocks;
        for (const auto& item : body["blocks"]) {
            if (item.t() != crow::json::type::Object || !item.has("text") || item["text"].t() != crow::json::type::String) {
                return ApiResponse::failure(400, "VALIDATION_ERROR", "Cada bloque debe ser un objeto con 'text'");
            }
            TranscriptBlockInput block;
            block.text = std::string(item["text"].s());
            if (item.has("speaker") && item["speaker"].t() == crow::json::type::String) {
                block.speaker = std::string(item["speaker"].s());
            }
            block.startSeconds = readOptionalNumber(item, "start");
            block.endSeconds = readOptionalNumber(item, "end");
            blocks.push_back(std::move(block));
        }

        return transcriptEditResponse(id, m_service.updateTranscript(id, blocks));

    } catch (const std::exception& e) {
        std::stringstream log_error_ss;
        log_error_ss << "[interviewController][updateTranscript] Fallo en updateTranscript. Detalle: " << e.what();
        log_event(log_error_ss.str());
        return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error guardando la transcripcion");
    }
}

crow::response InterviewController::restoreTranscript(int id) {
    try {
        return transcriptEditResponse(id, m_service.restoreTranscript(id));
    } catch (const std::exception& e) {
        std::stringstream log_error_ss;
        log_error_ss << "[interviewController][restoreTranscript] Fallo en restoreTranscript. Detalle: " << e.what();
        log_event(log_error_ss.str());
        return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error restaurando la transcripcion");
    }
}

crow::response InterviewController::processInterview(const crow::request& req, int id) {
    try {
        // Body opcional: {"include_summary": true, "enhance_transcript": true}.
        // Sin body, JSON invalido, o un campo ausente -> false (ambas fases
        // son opt-in explicito, ver Job.h). No es un error de la request, es
        // simplemente el default.
        bool includeSummary = false;
        bool enhanceTranscript = false;
        if (!req.body.empty()) {
            crow::json::rvalue body_json = crow::json::load(req.body);
            if (body_json && body_json.has("include_summary")) {
                includeSummary = body_json["include_summary"].b();
            }
            if (body_json && body_json.has("enhance_transcript")) {
                enhanceTranscript = body_json["enhance_transcript"].b();
            }
        }

        switch (m_service.requestProcessing(id, includeSummary, enhanceTranscript)) {
            case ProcessOutcome::NotFound:
                return ApiResponse::failure(404, "NOT_FOUND", "No existe una entrevista con ese id");

            case ProcessOutcome::AudioRequired:
                return ApiResponse::failure(409, "AUDIO_REQUIRED", "La entrevista todavia no tiene un audio asociado");

            case ProcessOutcome::AlreadyQueued:
                return ApiResponse::failure(409, "JOB_ALREADY_QUEUED", "Ya hay un procesamiento en curso para esta entrevista");

            case ProcessOutcome::Failed:
                return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error actualizando el estado de la entrevista");

            case ProcessOutcome::Ok: {
                crow::json::wvalue data;
                data["id"] = id;
                data["status"] = "processing";
                data["include_summary"] = includeSummary;
                data["enhance_transcript"] = enhanceTranscript;
                return ApiResponse::success(202, std::move(data));
            }
        }

        return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error procesando la entrevista");

    } catch (const std::exception& e) {
        std::stringstream log_error_ss;
        log_error_ss << "[interviewController][processInterview] Fallo en processInterview. Detalle: " << e.what();
        log_event(log_error_ss.str());
        return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error procesando la entrevista");
    }
}

crow::response InterviewController::deleteInterview(int id) {
    try {
        switch (m_service.removeInterview(id)) {
            case RemoveOutcome::NotFound:
                return ApiResponse::failure(404, "NOT_FOUND", "No existe una entrevista con ese id");

            case RemoveOutcome::Failed:
                return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error eliminando la entrevista");

            case RemoveOutcome::Ok: {
                crow::json::wvalue data;
                data["id"] = id;
                data["code"] = "DELETED";
                return ApiResponse::success(200, std::move(data));
            }
        }

        return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error eliminando la entrevista");

    } catch (const std::exception& e) {
        std::stringstream log_error_ss;
        log_error_ss << "[interviewController][deleteInterview] Fallo en deleteInterview. Detalle: " << e.what();
        log_event(log_error_ss.str());
        return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error eliminando la entrevista");
    }
}

crow::response InterviewController::updateKeywords(const crow::request& req, int id) {
    try {
        // Body: {"keywords": ["SIGGES", "GES", ...]}. El parseo del archivo
        // .txt lo hace el frontend; aca solo se valida la forma del JSON.
        crow::json::rvalue body_json = crow::json::load(req.body);
        if (!body_json || !body_json.has("keywords") || body_json["keywords"].t() != crow::json::type::List) {
            return ApiResponse::failure(400, "INVALID_KEYWORDS", "Se esperaba {\"keywords\": [\"termino\", ...]}");
        }
        std::vector<std::string> keywords;
        for (const auto& item : body_json["keywords"]) {
            if (item.t() != crow::json::type::String) {
                return ApiResponse::failure(400, "INVALID_KEYWORDS", "Cada palabra clave debe ser un texto");
            }
            keywords.emplace_back(item.s());
        }

        auto outcome = m_service.setKeywords(id, keywords);
        switch (outcome.status) {
            case KeywordsOutcome::Status::NotFound:
                return ApiResponse::failure(404, "NOT_FOUND", "No existe una entrevista con ese id");

            case KeywordsOutcome::Status::Invalid:
                return ApiResponse::failure(400, "INVALID_KEYWORDS", outcome.message);

            case KeywordsOutcome::Status::Failed:
                return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error guardando las palabras clave");

            case KeywordsOutcome::Status::Ok: {
                std::vector<crow::json::wvalue> saved;
                for (const auto& keyword : outcome.keywords) {
                    saved.emplace_back(keyword);
                }
                crow::json::wvalue data;
                data["id"] = id;
                data["keywords"] = std::move(saved);
                return ApiResponse::success(200, std::move(data));
            }
        }

        return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error guardando las palabras clave");

    } catch (const std::exception& e) {
        std::stringstream log_error_ss;
        log_error_ss << "[interviewController][updateKeywords] Fallo en updateKeywords. Detalle: " << e.what();
        log_event(log_error_ss.str());
        return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error guardando las palabras clave");
    }
}

#include "crow.h"
#include "../include/interviewController.h"
#include "../include/InterviewControllerHelper.h"
#include "../include/ApiResponse.h"
#include "../include/logger.h"
#include <vector>

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
            data["result"] = std::move(result);
        } else {
            data["result"] = nullptr;
        }

        return ApiResponse::success(200, std::move(data));

    } catch (const std::exception& e) {
        std::stringstream log_error_ss;
        log_error_ss << "[interviewController][getInterview] Fallo en getInterview. Detalle: " << e.what();
        log_event(log_error_ss.str());
        return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error obteniendo la entrevista");
    }
}

crow::response InterviewController::processInterview(int id) {
    try {
        switch (m_service.requestProcessing(id)) {
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

#include "crow.h"
#include "../include/interviewController.h"
#include "../include/InterviewControllerHelper.h"
#include "../include/ApiResponse.h"
#include "../include/logger.h"

crow::response InterviewController::handleInterviewRegistration(const crow::request& req) {
    try {
        crow::json::rvalue body_json = crow::json::load(req.body);
        auto [date, type, subject_type] = getFields(body_json);
        std::stringstream log_ss;
        log_ss << "[Helpers][validateFileInformation]  Archivo recibido con exito. "
            << "name: "         << date         << "  |  " << "\n"
            << "size: "         << type         << "  |  " << "\n"
            << "subject_type: " << subject_type << "  |  " << "\n";
        log_event(log_ss.str());

        crow::json::wvalue data;
        data["code"] = "CREATED";
        return ApiResponse::success(201, std::move(data));
    } catch (std::exception& e) {
        std::stringstream log_error_ss;
        log_error_ss << "[interviewController][handleInterviewRegistration] Fallo en handleInterviewRegistration. Detalle: " << e.what();
        log_event(log_error_ss.str());
        return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error registrando entrevista");
    }
}
#include "crow.h"
#include "../include/interviewController.h"
#include "../include/InterviewControllerHelper.h"
#include "../include/ApiResponse.h"
#include "../include/logger.h"
#include "../include/DatabaseManager.h"
#include <vector>

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
        

        // continuar con la logica de almacenado de datos en MySql
        std::string query = "INSERT INTO interviews (interview_date, interview_type, interview_subject_type, created_at, updated_at) VALUES (?, ?, ?, NOW(), NOW())";
        std::vector<SqlParam> params = {date, type, subject_type};
        
        auto& db = DatabaseManager::getInstance();

        if (db.executePrepared(query, params)) {
            crow::json::wvalue data;
            data["code"] = "CREATED";
            return ApiResponse::success(201, std::move(data));
        }

        std::stringstream log_error_ss;
        log_error_ss << "[interviewController][handleInterviewRegistration] Fallo en handleInterviewRegistration. Error al almacenar la informacion de entrevista";
        log_event(log_error_ss.str());
        return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error registrando entrevista");
        
    } catch (std::exception& e) {
        std::stringstream log_error_ss;
        log_error_ss << "[interviewController][handleInterviewRegistration] Fallo en handleInterviewRegistration. Detalle: " << e.what();
        log_event(log_error_ss.str());
        return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error registrando entrevista");
    }
}
#include "crow.h"
#include "../include/interviewController.h"
#include "../include/InterviewControllerHelper.h"
#include "../include/ApiResponse.h"
#include "../include/logger.h"
#include "../include/DatabaseManager.h"
#include <vector>
#include <cstdint>

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
        auto result = db.executePrepared(query, params, true);
        if (result.has_value() && result.value() > 0) {
            uint64_t row_id = result.value();
            crow::json::wvalue data;
            data["code"] = "CREATED";
            data["id"] = row_id;
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

crow::response InterviewController::getInterviews(const crow::request& req) {
    try {
        std::string query =
            "SELECT id, interview_date, interview_type, interview_subject_type, status, created_at "
            "FROM interviews ORDER BY id DESC";

        auto& db = DatabaseManager::getInstance();
        auto rows = db.executeQuery(query);

        std::vector<crow::json::wvalue> interviews;
        interviews.reserve(rows.size());

        for (const auto& row : rows) {
            if (row.size() != 6) {
                log_event("[interviewController][getInterviews] Fila con cantidad de columnas inesperada, se omite");
                continue;
            }

            crow::json::wvalue interview;
            interview["id"] = std::get<int>(row[0]);
            interview["date"] = std::get<std::string>(row[1]);
            interview["type"] = std::get<std::string>(row[2]);
            interview["subject_type"] = std::get<std::string>(row[3]);
            interview["status"] = std::get<std::string>(row[4]);
            interview["created_at"] = std::get<std::string>(row[5]);
            interviews.push_back(std::move(interview));
        }

        crow::json::wvalue data;
        data = std::move(interviews);
        return ApiResponse::success(200, std::move(data));

    } catch (const std::exception& e) {
        std::stringstream log_error_ss;
        log_error_ss << "[interviewController][getInterviews] Fallo en getInterviews. Detalle: " << e.what();
        log_event(log_error_ss.str());
        return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error obteniendo el listado de entrevistas");
    }
}

crow::response InterviewController::getInterview(int id) {
    try {
        auto& db = DatabaseManager::getInstance();
        std::vector<SqlParam> idParam = {id};

        auto interviewRows = db.executeQuery(
            "SELECT id, interview_date, interview_type, interview_subject_type, status, created_at "
            "FROM interviews WHERE id = ?",
            idParam);

        if (interviewRows.empty()) {
            return ApiResponse::failure(404, "NOT_FOUND", "No existe una entrevista con ese id");
        }

        const auto& interviewRow = interviewRows[0];
        if (interviewRow.size() != 6) {
            log_event("[interviewController][getInterview] Fila de entrevista con cantidad de columnas inesperada");
            return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error obteniendo la entrevista");
        }

        crow::json::wvalue data;
        data["id"] = std::get<int>(interviewRow[0]);
        data["date"] = std::get<std::string>(interviewRow[1]);
        data["type"] = std::get<std::string>(interviewRow[2]);
        data["subject_type"] = std::get<std::string>(interviewRow[3]);
        data["status"] = std::get<std::string>(interviewRow[4]);
        data["created_at"] = std::get<std::string>(interviewRow[5]);

        // interviews_audio / interview_results tienen UNIQUE(interview_id):
        // a lo sumo una fila cada una.
        auto audioRows = db.executeQuery(
            "SELECT interview_audio_path, interview_audio_format, interview_audio_size "
            "FROM interviews_audio WHERE interview_id = ?",
            idParam);

        if (!audioRows.empty() && audioRows[0].size() == 3) {
            crow::json::wvalue audio;
            audio["path"] = std::get<std::string>(audioRows[0][0]);
            audio["format"] = std::get<std::string>(audioRows[0][1]);
            audio["size"] = std::get<long long>(audioRows[0][2]);
            data["audio"] = std::move(audio);
        } else {
            data["audio"] = nullptr;
        }

        auto resultRows = db.executeQuery(
            "SELECT interview_transcription_file_path "
            "FROM interview_results WHERE interview_id = ?",
            idParam);

        if (!resultRows.empty() && resultRows[0].size() == 1) {
            crow::json::wvalue result;
            result["transcription_file_path"] = std::get<std::string>(resultRows[0][0]);
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
        auto& db = DatabaseManager::getInstance();
        std::vector<SqlParam> idParam = {id};

        auto interviewRows = db.executeQuery("SELECT id FROM interviews WHERE id = ?", idParam);
        if (interviewRows.empty()) {
            return ApiResponse::failure(404, "NOT_FOUND", "No existe una entrevista con ese id");
        }

        auto audioRows = db.executeQuery("SELECT id FROM interviews_audio WHERE interview_id = ?", idParam);
        if (audioRows.empty()) {
            return ApiResponse::failure(409, "AUDIO_REQUIRED", "La entrevista todavia no tiene un audio asociado");
        }

        // No hay cola de trabajos ni Whisper integrado todavia (Sprint 4/5 del
        // roadmap): esto solo deja constancia de que se pidio procesar.
        auto updateResult = db.executePrepared(
            "UPDATE interviews SET status = 'processing' WHERE id = ?", idParam);

        if (!updateResult.has_value()) {
            log_event("[interviewController][processInterview] Fallo actualizando el estado a processing");
            return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error actualizando el estado de la entrevista");
        }

        crow::json::wvalue data;
        data["id"] = id;
        data["status"] = "processing";
        return ApiResponse::success(202, std::move(data));

    } catch (const std::exception& e) {
        std::stringstream log_error_ss;
        log_error_ss << "[interviewController][processInterview] Fallo en processInterview. Detalle: " << e.what();
        log_event(log_error_ss.str());
        return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error procesando la entrevista");
    }
}
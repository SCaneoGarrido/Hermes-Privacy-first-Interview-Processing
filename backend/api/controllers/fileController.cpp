#include "crow.h"
#include "../include/FileController.h"
#include "../include/ApiResponse.h"
#include "../include/logger.h"
#include "../include/FileControllerHelper.h"
#include "../include/DatabaseManager.h"
#include <string>
#include <sstream>
#include <iostream>
#include <vector>

crow::response FileController::handleFileUpload(const crow::request& req, int& interview_id) {
    try {
        // Analizo el contenido multipart de la solicitud para extraer el archivo
        crow::multipart::message file_form(req);

        auto [filename, file_content, bytes_size] = validateFileInformation(file_form);
        if (filename.empty() || file_content.empty()) {
           return ApiResponse::failure(400, "INVALID_FILE", "Error en la solicitud");
        }

        std::string ext = getFileExtension(filename);
        auto [fileSaved, savedPath] = saveFile(file_content, ext);
        if (!fileSaved) {
            return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error interno del servidor");
        }

        std::string query = "INSERT INTO interviews_audio (interview_audio_path, interview_audio_format, interview_audio_size, interview_id, created_at) VALUES (?, ?, ?, ?, NOW())";
        std::vector<SqlParam> params = {savedPath, ext, bytes_size, interview_id};

        auto& db = DatabaseManager::getInstance();
        if (!db.executePrepared(query, params)) {
            log_event("[fileController][handleFileUpload] Fallo al registrar el audio en la BD");
            return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error interno del servidor");
        }

        // La entrevista ya tiene audio: pasa de "pending_audio" a "pending_processing".
        // No aborta el request si esto falla (el audio ya quedo guardado); solo se loggea.
        std::vector<SqlParam> statusParams = {interview_id};
        if (!db.executePrepared("UPDATE interviews SET status = 'pending_processing' WHERE id = ?", statusParams)) {
            log_event("[fileController][handleFileUpload] Fallo actualizando el status de la entrevista a pending_processing");
        }

        crow::json::wvalue data;
        data["filename"] = filename;
        data["path"] = savedPath;
        data["code"] = "ACCEPTED";
        return ApiResponse::success(202, std::move(data));

    } catch (const std::exception& e) {
        // --- CONSTRUCCIÓN DEL LOG DE ERROR ---
        std::stringstream log_error_ss;
        log_error_ss << "[fileController][handleFileUpload] Fallo en handleFileUpload. Detalle: " << e.what();

        log_event(log_error_ss.str());

        return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error al subir el archivo");
    }
}

crow::response FileController::getFileInfo(const std::string& file_id) {
    // Implementación de la lógica para obtener la información de un archivo
    // Aquí puedes buscar el archivo por su ID y devolver su información
    crow::json::wvalue data;
    data["file_id"] = file_id;
    return ApiResponse::success(200, std::move(data));
}
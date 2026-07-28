#include "crow.h"
#include "../include/FileController.h"
#include "../include/ApiResponse.h"
#include "../include/logger.h"
#include "../include/FileControllerHelper.h"
#include <string>
#include <sstream>
#include <iostream>
#include <vector>

FileController::FileController(InterviewService& service) : m_service(service) {}

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

        if (!m_service.attachAudio(interview_id, savedPath, ext, bytes_size)) {
            return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error interno del servidor");
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
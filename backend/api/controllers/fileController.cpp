#include "crow.h"
#include "../include/FileController.h"
#include "../include/ApiResponse.h"
#include "../include/logger.h"
#include "../include/FileControllerHelper.h"
#include <string>
#include <sstream>
#include <iostream>


crow::response FileController::handleFileUpload(const crow::request& req) {
    try {
        // Analizo el contenido multipart de la solicitud para extraer el archivo
        crow::multipart::message file_form(req);

        auto [filename, file_content] = validateFileInformation(file_form);
        if (filename.empty() || file_content.empty()) {
           return ApiResponse::failure(400, "INVALID_FILE", "Error en la solicitud");
        }

        std::string ext = getFileExtension(filename);
        bool fileSaved = saveFile(file_content, ext);
        if (!fileSaved) {
            return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error interno del servidor");
        }

        crow::json::wvalue data;
        data["filename"] = filename;
        return ApiResponse::success(202, std::move(data));

    } catch (const std::exception& e) {
        // --- CONSTRUCCIÓN DEL LOG DE ERROR ---
        std::stringstream log_error_ss;
        log_error_ss << "[ERROR] Fallo en handleFileUpload. Detalle: " << e.what();

        log_event(log_error_ss.str());

        return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error al subir el archivo: " + std::string(e.what()));
    }
}

crow::response FileController::getFileInfo(const std::string& file_id) {
    // Implementación de la lógica para obtener la información de un archivo
    // Aquí puedes buscar el archivo por su ID y devolver su información
    crow::json::wvalue data;
    data["file_id"] = file_id;
    return ApiResponse::success(200, std::move(data));
}
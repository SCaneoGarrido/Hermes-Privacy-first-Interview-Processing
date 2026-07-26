#ifndef FILE_FORMAT_GUARD_H
#define FILE_FORMAT_GUARD_H

#include "crow.h"
#include "ApiResponse.h"
#include "Constanst.h"
#include "logger.h"
#include <vector>
#include <string>

// Al usar "auto& ctx" este metodo es una plantilla: su definicion debe quedar
// visible aqui (no en un .cpp separado), o el linker no encuentra la instanciacion.
struct FileFormatGuard : crow::ILocalMiddleware {
    struct context {};

    // Crow invoca este metodo (por nombre) antes del handler de la ruta.
    // Valida el Content-Type real del archivo enviado en el campo "file".
    void before_handle(crow::request& req, crow::response& res, auto& ctx) {
        try {
            // El Content-Type del request es solo el "sobre" (multipart/form-data);
            // el formato real a validar es el de la parte "file" dentro del multipart.
            crow::multipart::message file_form(req);
            auto file_part = file_form.get_part_by_name("file");
            auto contentType = file_part.get_header_object("Content-Type").value;
            // VALIDACIÓN 1: ¿Existe el cuerpo del archivo?
            if (file_part.body.empty()) {
                std::stringstream log_error_ss;
                log_error_ss << "[Helpers][validateFileInformation] - 'file' not found in the request";
                log_event(log_error_ss.str());
                res = ApiResponse::failure(400, "BAD_REQUEST", "No existe informacion del archivo");
                res.end();
                return;
            }

            // VALIDACIÓN 2: ¿Tiene parámetros el header?
            if (contentType.params.empty()) {
                std::stringstream log_error_ss;
                log_error_ss << "[Helpers][validateFileInformation] - the request has empty params";
                log_event(log_error_ss.str());
                res = ApiResponse::failure(400, "BAD_REQUEST", "Headers sin parametros");
                res.end();
                return;
            }

            bool isValidFormat = false;
            for (const auto& formato : Config::FORMATOS_AUDIO_PERMITIDOS) {
                if (contentType == formato) {
                    isValidFormat = true;
                    break;
                }
            }

            if (!isValidFormat) {
                res = ApiResponse::failure(400, "UNSUPPORTED_MEDIA_TYPE", "Formato de archivo no permitido");
                res.end();
                return;
            }
        } catch (const std::exception& e) {
            res = ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error al validar el formato del archivo: " + std::string(e.what()));
            res.end();
        }
    }

    // Requerido por Crow para cualquier middleware registrado en crow::App<...>, no-op aqui.
    void after_handle(crow::request& req, crow::response& res, auto& ctx) {}
};
#endif // FILE_FORMAT_GUARD_H
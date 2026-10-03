#ifndef FILE_FORMAT_GUARD_H
#define FILE_FORMAT_GUARD_H

#include "crow.h"
#include "../ApiResponse.h"
#include "../AudioSignature.h"
#include "../Constanst.h"
#include "../logger.h"
#include <algorithm>
#include <cctype>
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
            // VALIDACIÓN 0: tope de tamaño, antes de parsear el multipart -- el
            // parseo copia el archivo completo, y con un video grande eso
            // multiplica el uso de RAM. Crow ya recibio el body a esta altura;
            // el frontend chequea el mismo tope antes de transferir.
            if (req.body.size() > Config::MAX_UPLOAD_BYTES) {
                std::stringstream log_error_ss;
                log_error_ss << "[middlewares][FileFormatGuard] - archivo rechazado por tamaño: " << req.body.size() << " bytes";
                log_event(log_error_ss.str());
                res = ApiResponse::failure(413, "PAYLOAD_TOO_LARGE", "El archivo supera el tamaño maximo permitido (1 GB)");
                res.end();
                return;
            }

            // El Content-Type del request es solo el "sobre" (multipart/form-data);
            // el formato real a validar es el de la parte "file" dentro del multipart.
            crow::multipart::message file_form(req);
            auto file_part = file_form.get_part_by_name("file");

            // VALIDACIÓN 1: ¿Existe el cuerpo del archivo?
            if (file_part.body.empty()) {
                std::stringstream log_error_ss;
                log_error_ss << "[middlewares][FileFormatGuard] - campo requerido 'file' no encontrado";
                log_event(log_error_ss.str());
                res = ApiResponse::failure(400, "BAD_REQUEST", "No existe informacion del archivo");
                res.end();
                return;
            }

            // Normalizamos el Content-Type (minusculas, sin espacios) para no depender
            // de que el cliente lo envie en un formato exacto (p.ej. "Audio/WAV " o "audio/wav ; codecs=1").
            std::string contentType = file_part.get_header_object("Content-Type").value;
            contentType.erase(std::remove_if(contentType.begin(), contentType.end(),
                                              [](unsigned char c) { return std::isspace(c); }),
                               contentType.end());
            std::transform(contentType.begin(), contentType.end(), contentType.begin(),
                            [](unsigned char c) { return std::tolower(c); });

            // VALIDACIÓN 2: ¿El formato del archivo esta permitido?
            bool isValidFormat = false;
            for (const auto& formato : Config::FORMATOS_PERMITIDOS) {
                if (contentType == formato) {
                    isValidFormat = true;
                    break;
                }
            }

            if (!isValidFormat) {
                std::stringstream log_error_ss;
                log_error_ss << "[middlewares][FileFormatGuard] - unsupported content type: '" << contentType << "'";
                log_event(log_error_ss.str());
                res = ApiResponse::failure(400, "UNSUPPORTED_MEDIA_TYPE", "Formato de archivo no permitido");
                res.end();
                return;
            }

            // VALIDACIÓN 3: ¿el contenido real coincide con el formato declarado?
            // El Content-Type lo pone el cliente (Postman, curl, etc.) y no prueba
            // nada por si solo -- cualquier texto plano pasa la validacion 2 si
            // el header dice "audio/wav". Esto chequea los bytes reales del archivo.
            if (!AudioSignature::isValid(contentType, file_part.body)) {
                std::stringstream log_error_ss;
                log_error_ss << "[middlewares][FileFormatGuard] - el contenido no coincide con el formato declarado '" << contentType << "'";
                log_event(log_error_ss.str());
                res = ApiResponse::failure(400, "INVALID_FILE_CONTENT", "El archivo no es un audio o video valido del formato declarado");
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
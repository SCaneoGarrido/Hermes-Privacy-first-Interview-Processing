#ifndef HEADER_ID_GUARD_H
#define HEADER_ID_GUARD_H
#include "crow.h"
#include "../ApiResponse.h"
#include "../logger.h"
#include <string>
#include <stdexcept>

struct HeaderIdGuard : crow::ILocalMiddleware {
    // Definimos la variable en el contexto para poder consumirla en el controlador
    struct context {
        int interview_id;
    };

    void before_handle(crow::request& req, crow::response& res, auto& ctx) {
        std::string raw_id = req.get_header_value("interview_id");

        if (raw_id.empty()) {
            log_event("[middlewares][HeaderIdGuard] Falta el encabezado 'interview_id'");
            res = ApiResponse::failure(400, "BAD_REQUEST", "Falta el encabezado 'interview_id'");
            res.end();
            return;
        }

        try {
            // Transformación y almacenamiento en el contexto
            ctx.interview_id = std::stoi(raw_id);
        } catch (...) {
            log_event("[middlewares][HeaderIdGuard] 'interview_id' no es un entero valido: " + raw_id);
            res = ApiResponse::failure(400, "BAD_REQUEST", "El 'interview_id' debe ser un numero entero valido");
            res.end();
            return;
        }
    }

    void after_handle(crow::request& req, crow::response& res, auto& ctx) {}
};
#endif // HEADER_ID_GUARD_H

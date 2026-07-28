#ifndef INTERVIEW_GUARD_H
#define INTERVIEW_GUARD_H
#include "crow.h"
#include "../ApiResponse.h"
#include "../logger.h"
#include <string>

struct InterviewGuard : crow::ILocalMiddleware {
    struct context {};

    void before_handle(crow::request& req, crow::response& res, auto& ctx) {
        try {
            crow::json::rvalue body_json = crow::json::load(req.body);
            if (!body_json) {
                std::stringstream log_error_ss;
                log_error_ss << "[middlewares][InterviewGuard] JSON Invalido o mal formado";
                log_event(log_error_ss.str());
                res = ApiResponse::failure(400, "BAD_REQUEST", "No existe informacion de entrevista"); 
                res.end();
                return;   
            }

            if (!body_json.has("date") || !body_json.has("type") || !body_json.has("subject_type")) {
                std::stringstream log_error_ss;
                log_error_ss << "[middlewares][InterviewGuard] Faltan campos requeridos 'date', 'type', 'subject_type";
                log_event(log_error_ss.str());
                res = ApiResponse::failure(400, "BAD_REQUEST", "Faltan campos requeridos: date, type, subject_type");
                res.end();
                return;
            }
        } catch (std::exception& e) {
            std::stringstream log_error_ss;
            log_error_ss << "[middlewares][InterviewGuard] - ERROR: '" << e.what();
            log_event(log_error_ss.str());
            res = ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error interno del servidor");
            res.end();
        }
    }

    void after_handle(crow::request& req, crow::response& res, auto& ctx) {}
};
#endif // INTERVIEW_GUARD_H
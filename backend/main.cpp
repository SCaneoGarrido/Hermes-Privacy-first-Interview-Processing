#include "crow.h"
#include "api/include/FileController.h"
#include "api/include/interviewController.h"
#include "api/include/middlewares/FileFormatGuard.h"
#include "api/include/middlewares/InterviewGuard.h"
#include "api/include/middlewares/HeaderIdGuard.h"
#include "api/include/DatabaseManager.h"
#include "api/include/Health.h"
#include "api/include/logger.h"
#include "api/include/ApiResponse.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

// Lee la config de conexion desde el entorno (ver .env / docker-compose.yml),
// con defaults razonables para desarrollo local si no estan seteadas.
std::string env_or(const char* name, const std::string& fallback) {
    const char* value = std::getenv(name);
    return value ? std::string(value) : fallback;
}

}  // namespace

int main()
{
    crow::App<FileFormatGuard, InterviewGuard, HeaderIdGuard>  app;
    FileController              fileController;
    InterviewController         interviewController;
    Health                      health;
    try {
        // Inicializas una sola vez al arrancar el backend
        DatabaseManager::initializeGlobal(
            env_or("MYSQL_HOST", "127.0.0.1"),
            env_or("MYSQL_USER", "hermes_app"),
            env_or("MYSQL_PASSWORD", "hermes_dev@123"),
            env_or("MYSQL_DATABASE", "hermes"),
            std::stoi(env_or("MYSQL_PORT", "3306")));
        DatabaseManager::getInstance().migrateTables("../SQL/init.sql");
    }
    catch (const DatabaseException& e) {
        log_event(std::string("[main] Fallo critico inicializando la base de datos: ") + e.what());
        std::cerr << "[CRITICAL ERROR] " << e.what() << std::endl;
        return 1;
    }
    // ==== Define route for service health check
    CROW_ROUTE(app, "/health")([&health](const crow::request& req) {
        return health.healthCheck(req);
    });

    // ==== Define routes for file upload and file info retrieval
    CROW_ROUTE(app, "/upload").methods(crow::HTTPMethod::POST).CROW_MIDDLEWARES(app, FileFormatGuard, HeaderIdGuard)([&fileController, &app](const crow::request& req) {
        auto& ctx = app.get_context<HeaderIdGuard>(req);
        return fileController.handleFileUpload(req, ctx.interview_id);
    });

    CROW_ROUTE(app, "/file/<string>").CROW_MIDDLEWARES(app, FileFormatGuard)([&fileController](const std::string& file_id) {
        return fileController.getFileInfo(file_id);
    });

    // ==== Define routes for interview logic
    CROW_ROUTE(app, "/interview").methods(crow::HTTPMethod::POST).CROW_MIDDLEWARES(app, InterviewGuard)([&interviewController](const crow::request& req) {
        return interviewController.handleInterviewRegistration(req);
    });

    // ==== Rutas/metodos sin match (404 / 405): sin esto, Crow devuelve su
    // respuesta por defecto (body vacio o texto plano), rompiendo el contrato.
    CROW_CATCHALL_ROUTE(app)([](const crow::request&, crow::response& res) {
        if (res.code == 405) {
            res = ApiResponse::failure(405, "METHOD_NOT_ALLOWED", "Metodo no permitido para este recurso");
        } else {
            res = ApiResponse::failure(404, "NOT_FOUND", "Recurso no encontrado");
        }
    });

    // ==== Red de seguridad: cualquier excepcion que se escape de un handler
    // (fuera de los try/catch de cada controller) tambien debe respetar el contrato.
    app.exception_handler([](crow::response& res) {
        try {
            throw;
        } catch (const crow::bad_request& e) {
            log_event(std::string("[main][exception_handler] Solicitud invalida no capturada: ") + e.what());
            res = ApiResponse::failure(400, "BAD_REQUEST", e.what());
        } catch (const std::exception& e) {
            log_event(std::string("[main][exception_handler] Excepcion no capturada: ") + e.what());
            res = ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error interno del servidor");
        } catch (...) {
            log_event("[main][exception_handler] Excepcion no capturada de tipo desconocido");
            res = ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error interno del servidor");
        }
    });

    app.port(18080).multithreaded().run();
}
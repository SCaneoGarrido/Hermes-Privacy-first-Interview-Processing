#include "crow.h"
#include "api/include/FileController.h"
#include "api/include/interviewController.h"
#include "api/include/middlewares/FileFormatGuard.h"
#include "api/include/middlewares/InterviewGuard.h"
#include "api/include/middlewares/HeaderIdGuard.h"
#include "api/include/DatabaseManager.h"
#include "api/include/repositories/MySqlInterviewRepository.h"
#include "api/include/services/InterviewService.h"
#include "api/include/Health.h"
#include "api/include/logger.h"
#include "api/include/ApiResponse.h"
#include "jobs/include/JobQueue.h"
#include "jobs/include/InterviewJobRepository.h"
#include "jobs/include/InterviewProcessingJobHandler.h"
#include "jobs/include/WorkerPool.h"
#include "audio/include/FfmpegAudioNormalizer.h"
#include "transcription/include/WhisperTranscriber.h"
#include "llm/include/OllamaClient.h"
#include "llm/include/TranscriptEnhancer.h"
#include "api/include/Constanst.h"

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

    // Composition root: controllers dependen solo de InterviewService, que a
    // su vez depende solo de IInterviewRepository (ver .ai/PROJECT.md,
    // Filosofia de Repositorios). DatabaseManager queda aislado dentro de
    // MySqlInterviewRepository y hermes::jobs::InterviewJobRepository.
    MySqlInterviewRepository    interviewRepository(DatabaseManager::getInstance());

    // Sprint 4 - Background Processing: cola de trabajos + worker threads.
    // Cualquier job que haya quedado 'running' de una corrida anterior murio
    // a mitad de proceso (el backend se reinicio); se reclama como failed
    // antes de aceptar trabajo nuevo, si no la entrevista queda bloqueada
    // para siempre.
    hermes::jobs::InterviewJobRepository        jobRepository(DatabaseManager::getInstance());
    jobRepository.reclaimStuckJobs();

    hermes::jobs::JobQueue                      jobQueue;

    // Sprint 5 - Whisper Integration: normalizacion (FFmpeg estatico, ver
    // ADR-014) + transcripcion (whisper.cpp). WhisperTranscriber no carga
    // el modelo aca -- lo hace perezosamente en el primer transcribe(), asi
    // el backend arranca igual aunque el modelo todavia no este en disco
    // (ver whisper.cpp Architecture en la vault).
    // Idioma fijo por defecto (no "auto"): la mayoria de las entrevistas de
    // Hermes son en espanol, y "auto" agrega una pasada extra de deteccion
    // que ademas es poco confiable en clips cortos/ruidosos (ver whisper.cpp
    // Architecture, Common Mistakes). Configurable por si alguna entrevista
    // puntual es en otro idioma.
    hermes::audio::FfmpegAudioNormalizer        audioNormalizer;
    hermes::transcription::WhisperTranscriber   transcriber(
        env_or("WHISPER_MODEL_PATH", std::string(Config::DEFAULT_WHISPER_MODEL_PATH)),
        env_or("WHISPER_LANGUAGE", "es"));

    // Sprint 6 - Ollama Integration: correccion+estructuracion,
    // anonimizacion y resumen sobre la transcripcion de Sprint 5. Sin
    // estado propio pesado (a diferencia de whisper_context, no hace
    // falta carga perezosa ni mutex - ver ADR-015).
    hermes::llm::OllamaClient                   ollamaClient(
        env_or("OLLAMA_BASE_URL", "http://localhost:11434"),
        env_or("OLLAMA_MODEL", "qwen2.5:7b"));
    hermes::llm::TranscriptEnhancer             transcriptEnhancer(ollamaClient);

    hermes::jobs::InterviewProcessingJobHandler jobHandler(interviewRepository, jobRepository, audioNormalizer, transcriber, transcriptEnhancer);
    const int workerPoolSize = std::stoi(env_or("WORKER_POOL_SIZE", "1"));
    hermes::jobs::WorkerPool                    workerPool(jobQueue, jobHandler, jobRepository, workerPoolSize);

    InterviewService             interviewService(interviewRepository, jobRepository, jobQueue);
    FileController               fileController(interviewService);
    InterviewController          interviewController(interviewService);
    // ==== Define route for service health check
    CROW_ROUTE(app, "/api/v1/health")([&health](const crow::request& req) {
        return health.healthCheck(req);
    });

    // ==== Define routes for file upload and file info retrieval
    CROW_ROUTE(app, "/api/v1/upload").methods(crow::HTTPMethod::POST).CROW_MIDDLEWARES(app, FileFormatGuard, HeaderIdGuard)([&fileController, &app](const crow::request& req) {
        auto& ctx = app.get_context<HeaderIdGuard>(req);
        return fileController.handleFileUpload(req, ctx.interview_id);
    });

    CROW_ROUTE(app, "/api/v1/file/<string>").CROW_MIDDLEWARES(app, FileFormatGuard)([&fileController](const std::string& file_id) {
        return fileController.getFileInfo(file_id);
    });

    // ==== Define routes for interview logic
    CROW_ROUTE(app, "/api/v1/interview").methods(crow::HTTPMethod::POST).CROW_MIDDLEWARES(app, InterviewGuard)([&interviewController](const crow::request& req) {
        return interviewController.handleInterviewRegistration(req);
    });

    CROW_ROUTE(app, "/api/v1/interviews").methods(crow::HTTPMethod::GET)([&interviewController](const crow::request& req) {
        return interviewController.getInterviews(req);
    });

    CROW_ROUTE(app, "/api/v1/interview/<int>").methods(crow::HTTPMethod::GET)([&interviewController](int id) {
        return interviewController.getInterview(id);
    });

    CROW_ROUTE(app, "/api/v1/interview/<int>").methods(crow::HTTPMethod::Delete)([&interviewController](int id) {
        return interviewController.deleteInterview(id);
    });

    CROW_ROUTE(app, "/api/v1/interview/<int>/process").methods(crow::HTTPMethod::POST)([&interviewController](const crow::request& req, int id) {
        return interviewController.processInterview(req, id);
    });

    CROW_ROUTE(app, "/api/v1/interview/<int>/download/transcript").methods(crow::HTTPMethod::GET)([&interviewController](int id) {
        return interviewController.downloadTranscript(id);
    });

    CROW_ROUTE(app, "/api/v1/interview/<int>/download/summary").methods(crow::HTTPMethod::GET)([&interviewController](int id) {
        return interviewController.downloadSummary(id);
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

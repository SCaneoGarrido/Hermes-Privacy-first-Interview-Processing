#include "crow.h"
#include "../include/Health.h"
#include "../include/ApiResponse.h"
#include "../include/logger.h"
#include <chrono>
#include <ctime>
#include <string>
#include <vector>

namespace {
    // Momento en que el proceso arranco, usado para calcular el uptime
    const std::chrono::steady_clock::time_point SERVER_START_TIME = std::chrono::steady_clock::now();

    // Describe un endpoint disponible en la API para exponerlo en /health
    crow::json::wvalue describeEndpoint(const std::string& method, const std::string& path, const std::string& description) {
        crow::json::wvalue endpoint;
        endpoint["method"] = method;
        endpoint["path"] = path;
        endpoint["description"] = description;
        return endpoint;
    }
}

crow::response Health::healthCheck(const crow::request& req) {
    try {
        const auto now = std::chrono::system_clock::now();
        const std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        const auto uptime_seconds = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - SERVER_START_TIME
        ).count();

        char timestamp[32];
        std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now_time));

        std::vector<crow::json::wvalue> endpoints;
        endpoints.push_back(describeEndpoint("GET", "/health", "Verifica el estado del servicio"));
        endpoints.push_back(describeEndpoint("POST", "/upload", "Sube un archivo de audio para su procesamiento"));
        endpoints.push_back(describeEndpoint("GET", "/file/<string>", "Obtiene la informacion de un archivo dado su ID"));

        crow::json::wvalue health_info;
        health_info["status"] = "ok";
        health_info["service"] = "Hermes API";
        health_info["timestamp"] = timestamp;
        health_info["uptime_seconds"] = uptime_seconds;
        health_info["endpoints"] = std::move(endpoints);

        return ApiResponse::success(200, std::move(health_info));
    } catch (const std::exception& e) {
        std::stringstream log_error_ss;
        log_error_ss << "[ERROR] Fallo en healthCheck. Detalle: " << e.what();
        log_event(log_error_ss.str());
        return ApiResponse::failure(500, "INTERNAL_SERVER_ERROR", "Error interno del servidor");
    }
}
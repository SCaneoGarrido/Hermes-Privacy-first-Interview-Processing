#ifndef API_RESPONSE_H
#define API_RESPONSE_H

#include "crow.h"
#include <string>

// Construye respuestas que cumplen el contrato definido en api/rules/contract.md:
// { "success": bool, "data": <objeto|array|null>, "error": {"code","message"} | null }
namespace ApiResponse {

    inline crow::response success(int http_code, crow::json::wvalue data = nullptr) {
        crow::json::wvalue body;
        body["success"] = true;
        body["data"] = std::move(data);
        body["error"] = nullptr;
        return crow::response(http_code, "application/json", body.dump(4));
    }

    inline crow::response failure(int http_code, const std::string& error_code, const std::string& message) {
        crow::json::wvalue error;
        error["code"] = error_code;
        error["message"] = message;

        crow::json::wvalue body;
        body["success"] = false;
        body["data"] = nullptr;
        body["error"] = std::move(error);
        return crow::response(http_code, "application/json", body.dump(4));
    }
}

#endif // API_RESPONSE_H

#include "crow.h"
#include "../include/ApiResponse.h"
#include "../include/logger.h"
#include "../include/InterviewControllerHelper.h"

std::tuple<std::string, std::string, std::string> getFields(crow::json::rvalue& body_json) {
    std::string date         = body_json["date"].s();
    std::string type         = body_json["type"].s();
    std::string subject_type = body_json["subject_type"].s();
    return {date, type, subject_type};
}

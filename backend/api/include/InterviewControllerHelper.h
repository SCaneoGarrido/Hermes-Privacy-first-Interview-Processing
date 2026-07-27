#ifndef INTERVIEW_CONTROLLER_HELPER_H
#define INTERVIEW_CONTROLLER_HELPER_H
#include "crow.h"
#include <string>
#include <tuple>


std::tuple<std::string, std::string, std::string> getFields(crow::json::rvalue& body_json);

#endif //INTERVIEW_CONTROLLER_HELPER_H
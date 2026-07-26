#ifndef FILE_CONTROLLER_HELPER_H
#define FILE_CONTROLLER_HELPER_H

#include "crow.h"
#include <string>
#include <tuple>

std::tuple<std::string, std::string> validateFileInformation(crow::multipart::message& file_form);
std::string generateUUID();
bool saveFile(std::string& filename);

#endif // FILE_CONTROLLER_HELPER_H

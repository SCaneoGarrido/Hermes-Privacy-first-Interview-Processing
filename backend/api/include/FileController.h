#ifndef FILE_CONTROLLER_H
#define FILE_CONTROLLER_H

#include "crow.h"
#include <string>

class FileController {
public:
    crow::response handleFileUpload(const crow::request& req);
    crow::response getFileInfo(const std::string& file_id);
};

#endif // FILE_CONTROLLER_H
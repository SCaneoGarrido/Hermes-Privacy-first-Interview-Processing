#ifndef FILE_CONTROLLER_H
#define FILE_CONTROLLER_H

#include "crow.h"
#include "services/InterviewService.h"
#include <string>

class FileController {
public:
    explicit FileController(InterviewService& service);

    crow::response handleFileUpload(const crow::request& req, int& interview_id);
    crow::response getFileInfo(const std::string& file_id);

private:
    InterviewService& m_service;
};

#endif // FILE_CONTROLLER_H

#include "crow.h"
#include "api/include/FileController.h"
#include "api/include/FileFormatGuard.h"
#include "api/include/Health.h"

int main()
{
    crow::App<FileFormatGuard> app;
    FileController fileController;
    Health health;

    // ==== Define route for service health check
    CROW_ROUTE(app, "/health")([&health](const crow::request& req) {
        return health.healthCheck(req);
    });

    // ==== Define routes for file upload and file info retrieval
    CROW_ROUTE(app, "/upload").methods(crow::HTTPMethod::POST).CROW_MIDDLEWARES(app, FileFormatGuard)([&fileController](const crow::request& req) {
        return fileController.handleFileUpload(req);
    });

    CROW_ROUTE(app, "/file/<string>").CROW_MIDDLEWARES(app, FileFormatGuard)([&fileController](const std::string& file_id) {
        return fileController.getFileInfo(file_id);
    });

    app.port(18080).multithreaded().run();
}
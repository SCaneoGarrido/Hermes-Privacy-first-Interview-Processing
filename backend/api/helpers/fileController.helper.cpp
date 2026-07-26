#include "crow.h"
#include "../include/FileControllerHelper.h"
#include "../include/logger.h"
#include "../include/Constanst.h"
#include <fstream>
#include <filesystem>
#include <boost/uuid/uuid.hpp> // MAIN UUID CLASS
#include <boost/uuid/uuid_generators.hpp> // GENERATORS
#include <boost/uuid/uuid_io.hpp> //STREAM input/output

std::tuple<std::string, std::string> validateFileInformation(crow::multipart::message& file_form) {
    std::string file_content = "";
    std::string filename     = "";
    auto file_part = file_form.get_part_by_name("file");
    file_content = file_part.body;
    auto header_info = file_part.get_header_object("Content-Disposition");
    filename = header_info.params.count("filename") ? header_info.params["filename"] : "Desconocido";
    
    // LOG EXITOSO
    std::stringstream log_ss;
    log_ss << "[Helpers][validateFileInformation]  Archivo recibido con exito. "
           << "name: " << filename << " | " << "\n"
           << "size: " << file_content.size() << " bytes." << "\n";
    log_event(log_ss.str());

    return {filename, file_content};
}

std::string generateUUID() {
    boost::uuids::random_generator gen;
    boost::uuids::uuid id = gen();
    std::string uuid_str = boost::uuids::to_string(id); // Converting UUID to Strings
    return uuid_str;
}

bool saveFile(std::string& filename) {
    try {
        std::string uuid = generateUUID(); 
        // Dynamic path 
        std::filesystem::path storage_dir(Config::UPLOAD_DIRECTORY);
        if (!std::filesystem::exists(storage_dir)) {
            std::filesystem::create_directory(storage_dir);
        }

        std::filesystem::path file_path = storage_dir / (uuid + ".tmp");
   

    } catch (const std::exception& e) {
        std::stringstream log_error_ss;
        log_error_ss << "[Helpers][validateFileInformation] - Error while saving file. Details:  " << e.what();
        log_event(log_error_ss.str());
        return false;
    }
    return true;
}

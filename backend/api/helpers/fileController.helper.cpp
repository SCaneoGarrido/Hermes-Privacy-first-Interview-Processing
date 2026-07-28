#include "crow.h"
#include "../include/FileControllerHelper.h"
#include "../include/logger.h"
#include "../include/Constanst.h"
#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <filesystem>
#include <boost/uuid/uuid.hpp> // MAIN UUID CLASS
#include <boost/uuid/uuid_generators.hpp> // GENERATORS
#include <boost/uuid/uuid_io.hpp> //STREAM input/output

std::tuple<std::string, std::string, int> validateFileInformation(crow::multipart::message& file_form) {
    std::string file_content = "";
    std::string filename     = "";
    auto file_part = file_form.get_part_by_name("file");
    file_content = file_part.body;
    auto header_info = file_part.get_header_object("Content-Disposition");
    filename = header_info.params.count("filename") ? header_info.params["filename"] : "Desconocido";
    int bytes_size = file_content.size();
    // LOG EXITOSO
    std::stringstream log_ss;
    log_ss << "[Helpers][validateFileInformation]  Archivo recibido con exito. "
           << "name: " << filename << " | " << "\n"
           << "size: " << bytes_size << " bytes." << "\n";
    log_event(log_ss.str());

    return {filename, file_content, bytes_size};
}

std::string generateUUID() {
    boost::uuids::random_generator gen;
    boost::uuids::uuid id = gen();
    std::string uuid_str = boost::uuids::to_string(id); // Converting UUID to Strings
    return uuid_str;
}


// este metodo puede devolver la extension como el tamaño del archivo
std::string getFileExtension(const std::string& filename) {
    std::filesystem::path file(filename);
    return file.extension().string();
}

std::tuple<bool, std::string> saveFile(std::string& file_content, std::string& ext) { // Deberia retornar la ruta donde se almaceno
    try {
        std::string uuid = generateUUID(); 
        // Dynamic path 
        std::filesystem::path storage_dir(Config::UPLOAD_DIRECTORY);
        if (!std::filesystem::exists(storage_dir)) {
            std::filesystem::create_directory(storage_dir);
        }

        std::filesystem::path file_path = storage_dir / (uuid + ext);
        // Continuar con el guardado del archivo 

        std::ofstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            std::stringstream log_error_ss;
            log_error_ss << "[Helpers][validateFileInformation] - Error cannot create file." << "\n"
                << "Details: \n" << "Path: " << file_path << std::endl;
            log_event(log_error_ss.str());    
            return {false, ""};
        }

        file.write(file_content.data(), file_content.size());
        file.close();
        return {true, file_path.string()};
    } catch (const std::exception& e) {
        std::stringstream log_error_ss;
        log_error_ss << "[Helpers][validateFileInformation] - Error while saving file. Details:  " << e.what();
        log_event(log_error_ss.str());
        return {false, ""};
    }
    
}

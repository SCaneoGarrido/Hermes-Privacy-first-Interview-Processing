#include "../../include/logger.h"
#include <fstream>


void log_event(const std::string& msg) {
    std::ofstream logs_file("API.log", std::ios::app);
    if(logs_file.is_open()) {
        logs_file << msg << "\n";
    }
}
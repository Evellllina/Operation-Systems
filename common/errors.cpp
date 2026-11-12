#include "errors.h"

void log_msg(const std::string& category, const std::string& message) {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::cout << std::put_time(&tm, "%H:%M:%S") 
              << " [MSG] " << category << ": " << message << std::endl;
}

void log_err(const std::string& category, const std::string& message) {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::cerr << std::put_time(&tm, "%H:%M:%S") 
              << " [ERR] " << category << ": " << message << std::endl;
}
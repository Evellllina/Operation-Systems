#include "os/process.h"
#include "common/comm.h"
#include <iostream>
#include <fstream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        log_err("Child", "Usage: child <filename> <id>");
        return 1;
    }
    
    std::string filename = argv[1];
    std::string id = argv[2];
    std::string category = "Child" + id;
    
    log_msg(category.c_str(), "Child started");
    
    std::ofstream outfile(filename);
    if (!outfile.is_open()) {
        log_err(category.c_str(), ("Cannot open file: " + filename).c_str());
        return 1;
    }
    
    std::string line;
    while (std::getline(std::cin, line)) {
        std::string processed = remove_vowels(line);
        outfile << processed << "\n";
        std::cout << "[Child " << id << "] " << processed << std::endl;
    }
    
    outfile.close();
    log_msg(category.c_str(), "Child finished");
    return 0;
}
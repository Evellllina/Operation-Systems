#include "os/process.h"
#include "common/comm.h"
#include <iostream>
#include <fstream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 4) {
        log_err("Child", "Usage: child <filename> <id> <channel>");
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

    try {
        // Подключаемся к отображаемому файлу, который создал родитель
        SharedChannel channel(argv[3], false);

        std::string line;
        while (channel.read(line)) {
            std::string processed = remove_vowels(line);
            outfile << processed << "\n";
            std::cout << "[Child " << id << "] " << processed << std::endl;
        }
    } catch (const std::exception&) {
        log_err(category.c_str(), "Shared memory error");
        return 1;
    }

    outfile.close();
    log_msg(category.c_str(), "Child finished");
    return 0;
}

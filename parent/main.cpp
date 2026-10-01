#include "os/process.h"
#include "common/comm.h"
#include <iostream>
#include <string>
#include <limits>

int main() {
    log_msg("Parent", "Parent started");
    
    std::string file1, file2;
    std::cout << "Enter filename for Child1: ";
    if (!(std::cin >> file1)) return 1;
    std::cout << "Enter filename for Child2: ";
    if (!(std::cin >> file2)) return 1;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    Pipe pipe1, pipe2;
    
    ProcessHandle h1 = start_process("./child/child", file1, "1", pipe1, pipe2);
    ProcessHandle h2 = start_process("./child/child", file2, "2", pipe2, pipe1);
    
    if (!h1.is_valid || !h2.is_valid) {
        log_err("Parent", "Failed to start children");
        return 1;
    }
    
    std::cout << "\nEnter strings (empty line to quit):\n";
    std::string line;
    int count = 0;
    
    while (std::getline(std::cin, line)) {
        if (line.empty()) break;
        
        count++;
        size_t len = line.length();
        
        if (len > 10) {
            log_msg("Parent", ("Line " + std::to_string(count) + " (len=" + std::to_string(len) + ") -> Child2").c_str());
            pipe2.write(line);
        } else {
            log_msg("Parent", ("Line " + std::to_string(count) + " (len=" + std::to_string(len) + ") -> Child1").c_str());
            pipe1.write(line);
        }
    }

    pipe1.close_write();
    pipe2.close_write();

    log_msg("Parent", "Waiting for children");
    ProcessResult r1 = wait_process(h1);
    ProcessResult r2 = wait_process(h2);
    
    log_msg("Parent", ("Child1 exit: " + std::to_string(r1.exit_code)).c_str());
    log_msg("Parent", ("Child2 exit: " + std::to_string(r2.exit_code)).c_str());
    log_msg("Parent", "Parent finished");
    
    return 0;
}
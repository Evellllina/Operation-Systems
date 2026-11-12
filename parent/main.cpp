#include "os/process.h"
#include "common/comm.h"
#include <iostream>
#include <string>
#include <limits>
#include <memory>

int main() {
    log_msg("Parent", "Parent started");

    std::string file1, file2;
    std::cout << "Enter filename for Child1: ";
    if (!(std::cin >> file1)) return 1;
    std::cout << "Enter filename for Child2: ";
    if (!(std::cin >> file2)) return 1;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    // Отображаемые файлы для обмена данными: по одному на каждого ребёнка
    std::unique_ptr<SharedChannel> channel1, channel2;
    try {
        channel1 = std::make_unique<SharedChannel>("channel1.mmap", true);
        channel2 = std::make_unique<SharedChannel>("channel2.mmap", true);
    } catch (const std::exception&) {
        log_err("Parent", "Failed to create shared memory");
        return 1;
    }

    ProcessHandle h1 = start_process("./child/child", file1, "1", channel1->path());
    ProcessHandle h2 = start_process("./child/child", file2, "2", channel2->path());

    if (!h1.is_valid || !h2.is_valid) {
        log_err("Parent", "Failed to start children");
        return 1;
    }
    channel1->set_peer(h1);
    channel2->set_peer(h2);

    // Если ребёнок перестал принимать данные, больше ему не пишем
    bool ok1 = true, ok2 = true;
    auto send = [](SharedChannel& channel, bool& ok, const std::string& data) {
        if (!ok) return;
        try {
            channel.write(data);
        } catch (const std::exception&) {
            ok = false;
            log_err("Parent", "Child does not accept data: " + channel.path());
        }
    };
    auto finish = [](SharedChannel& channel, bool& ok) {
        if (!ok) return;
        try {
            channel.finish();
        } catch (const std::exception&) {
            ok = false;
        }
    };

    std::cout << "\nEnter strings (empty line to quit):\n";
    std::string line;
    int count = 0;

    while (std::getline(std::cin, line)) {
        if (line.empty()) break;

        count++;
        size_t len = line.length();

        if (len > 10) {
            log_msg("Parent", ("Line " + std::to_string(count) + " (len=" + std::to_string(len) + ") -> Child2").c_str());
            send(*channel2, ok2, line);
        } else {
            log_msg("Parent", ("Line " + std::to_string(count) + " (len=" + std::to_string(len) + ") -> Child1").c_str());
            send(*channel1, ok1, line);
        }
    }

    finish(*channel1, ok1);
    finish(*channel2, ok2);

    log_msg("Parent", "Waiting for children");
    ProcessResult r1 = wait_process(h1);
    ProcessResult r2 = wait_process(h2);

    log_msg("Parent", ("Child1 exit: " + std::to_string(r1.exit_code)).c_str());
    log_msg("Parent", ("Child2 exit: " + std::to_string(r2.exit_code)).c_str());
    log_msg("Parent", "Parent finished");

    return 0;
}

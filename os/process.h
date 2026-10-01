#pragma once
#include "common/defines.h"

struct ProcessResult {
    int exit_code = 0;
};

struct ProcessHandle {
#ifdef OS_WIN
    HANDLE process = nullptr;
    HANDLE thread = nullptr;
#else
    pid_t pid = -1;
#endif
    bool is_valid = false;
};

class Pipe {
public:
    Pipe();
    ~Pipe();
    
    void write(const std::string& data);
    void close_read();
    void close_write();
    
#ifdef OS_WIN
    HANDLE get_read_handle() const { return read_handle_; }
    HANDLE get_write_handle() const { return write_handle_; }
#else
    int get_read_fd() const { return fd_[0]; }
    int get_write_fd() const { return fd_[1]; }
#endif
    
private:
#ifdef OS_WIN
    HANDLE read_handle_ = nullptr;
    HANDLE write_handle_ = nullptr;
#else
    int fd_[2] = {-1, -1};
#endif
};

ProcessHandle start_process(const std::string& exe, const std::string& arg1, const std::string& arg2, Pipe& stdin_pipe, Pipe& other_pipe);
ProcessResult wait_process(ProcessHandle& handle);
std::string remove_vowels(const std::string& str);
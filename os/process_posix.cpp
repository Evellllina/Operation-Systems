#include "process.h"
#include "common/comm.h"
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
//создаёт канал
Pipe::Pipe() {
    if (::pipe(fd_) < 0) {
        log_err("Pipe", "pipe() failed");
        throw std::runtime_error("pipe failed");
    }
    log_msg("Pipe", "Created POSIX pipe");
}
//автоматически закрывает канал
Pipe::~Pipe() {
    close_read();
    close_write();
}
//Запись данных в канал
void Pipe::write(const std::string& data) {
    std::string to_send = data + "\n";
    ssize_t w = ::write(fd_[1], to_send.c_str(), to_send.size());
    if (w < 0) log_err("Pipe", "write() failed");
}
//Закрытие конца для чтения
void Pipe::close_read() {
    if (fd_[0] >= 0) { ::close(fd_[0]); fd_[0] = -1; }
}
//Закрытие конца для записи
void Pipe::close_write() {
    if (fd_[1] >= 0) { ::close(fd_[1]); fd_[1] = -1; }
}

//запуск дочернего процесса
ProcessHandle start_process(const std::string& exe, const std::string& arg1, const std::string& arg2, Pipe& stdin_pipe, Pipe& other_pipe) {
    ProcessHandle h;
    pid_t pid = fork();
    
    if (pid < 0) {
        log_err("Process", "fork() failed");
        return h;
    }
    
    if (pid == 0) {

        ::dup2(stdin_pipe.get_read_fd(), STDIN_FILENO);
        ::close(stdin_pipe.get_read_fd());

        stdin_pipe.close_write();
        
        other_pipe.close_read();
        other_pipe.close_write();
        
        execl(exe.c_str(), exe.c_str(), arg1.c_str(), arg2.c_str(), nullptr);
        perror("execl");
        _exit(127);
    }

    h.pid = pid;
    h.is_valid = true;
    stdin_pipe.close_read();
    log_msg("Process", "Child started (PID: " + std::to_string(pid) + ")");
    return h;
}


ProcessResult wait_process(ProcessHandle& handle) {
    ProcessResult res;
    if (!handle.is_valid) return res;
    
    int status = 0;
    waitpid(handle.pid, &status, 0);
    res.exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    handle.is_valid = false;
    return res;
}
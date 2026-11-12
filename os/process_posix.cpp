#include "process.h"
#include "common/comm.h"
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <algorithm>
#include <stdexcept>

namespace {

std::string sys_error(const std::string& call) {
    return call + " failed: " + std::strerror(errno);
}

// Ожидание семафора; повтор, если ожидание прервал сигнал
void sem_wait_checked(sem_t* sem) {
    while (sem_wait(sem) < 0) {
        if (errno != EINTR) {
            log_err("Channel", sys_error("sem_wait()"));
            throw std::runtime_error("sem_wait failed");
        }
    }
}

void sem_post_checked(sem_t* sem) {
    if (sem_post(sem) < 0) {
        log_err("Channel", sys_error("sem_post()"));
        throw std::runtime_error("sem_post failed");
    }
}

}

//создаёт (родитель) или открывает (ребёнок) файл и отображает его в память
SharedChannel::SharedChannel(const std::string& path, bool owner) : path_(path), owner_(owner) {
    int flags = owner ? (O_RDWR | O_CREAT | O_TRUNC) : O_RDWR;
    fd_ = ::open(path.c_str(), flags, 0600);
    if (fd_ < 0) {
        log_err("Channel", sys_error("open(" + path + ")"));
        throw std::runtime_error("open failed");
    }

    if (owner && ::ftruncate(fd_, sizeof(SharedBlock)) < 0) {
        log_err("Channel", sys_error("ftruncate()"));
        ::close(fd_);
        throw std::runtime_error("ftruncate failed");
    }

    void* addr = ::mmap(nullptr, sizeof(SharedBlock), PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
    if (addr == MAP_FAILED) {
        log_err("Channel", sys_error("mmap()"));
        ::close(fd_);
        throw std::runtime_error("mmap failed");
    }
    block_ = static_cast<SharedBlock*>(addr);

    if (owner) {
        block_->finished = 0;
        block_->last = 0;
        block_->len = 0;
        // второй аргумент 1 — семафор общий для нескольких процессов
        if (sem_init(&block_->ready, 1, 0) < 0 || sem_init(&block_->empty, 1, 1) < 0) {
            log_err("Channel", sys_error("sem_init()"));
            ::munmap(block_, sizeof(SharedBlock));
            ::close(fd_);
            throw std::runtime_error("sem_init failed");
        }
        log_msg("Channel", "Created memory-mapped file " + path);
    }
}

//снимает отображение; родитель ещё удаляет семафоры и файл
SharedChannel::~SharedChannel() {
    if (block_) {
        if (owner_) {
            sem_destroy(&block_->ready);
            sem_destroy(&block_->empty);
        }
        ::munmap(block_, sizeof(SharedBlock));
    }
    if (fd_ >= 0) ::close(fd_);
    if (owner_) ::unlink(path_.c_str());
}

//Ожидание свободного буфера с проверкой, что ребёнок ещё работает
void SharedChannel::wait_empty() {
    while (true) {
        timespec deadline{};
        clock_gettime(CLOCK_REALTIME, &deadline);
        deadline.tv_nsec += 100 * 1000 * 1000;  // 100 мс
        if (deadline.tv_nsec >= 1000000000L) {
            deadline.tv_sec += 1;
            deadline.tv_nsec -= 1000000000L;
        }

        if (sem_timedwait(&block_->empty, &deadline) == 0) return;
        if (errno == EINTR) continue;
        if (errno != ETIMEDOUT) {
            log_err("Channel", sys_error("sem_timedwait()"));
            throw std::runtime_error("sem_timedwait failed");
        }

        // Ребёнок завершился, не дочитав данные? WNOWAIT — не забирать его статус
        if (peer_.is_valid) {
            siginfo_t info{};
            if (waitid(P_PID, peer_.pid, &info, WEXITED | WNOHANG | WNOWAIT) == 0 &&
                info.si_pid == peer_.pid) {
                log_err("Channel", "Child exited, " + path_ + " is not read anymore");
                throw std::runtime_error("peer exited");
            }
        }
    }
}

//Запись строки в общую память; длинная строка уходит несколькими порциями
void SharedChannel::write(const std::string& line) {
    size_t pos = 0;
    do {
        size_t n = std::min(line.size() - pos, SHM_DATA_SIZE);
        wait_empty();
        std::memcpy(block_->data, line.data() + pos, n);
        block_->len = n;
        pos += n;
        block_->last = (pos == line.size());
        block_->finished = 0;
        sem_post_checked(&block_->ready);
    } while (pos < line.size());
}

//Сигнал ребёнку, что строк больше не будет
void SharedChannel::finish() {
    wait_empty();
    block_->finished = 1;
    sem_post_checked(&block_->ready);
}

//Чтение строки из общей памяти; false — родитель закончил передачу
bool SharedChannel::read(std::string& line) {
    line.clear();
    while (true) {
        sem_wait_checked(&block_->ready);
        if (block_->finished) return false;
        line.append(block_->data, block_->len);
        bool last = block_->last;
        sem_post_checked(&block_->empty);
        if (last) return true;
    }
}

//запуск дочернего процесса
ProcessHandle start_process(const std::string& exe, const std::string& arg1, const std::string& arg2, const std::string& arg3) {
    ProcessHandle h;
    pid_t pid = fork();

    if (pid < 0) {
        log_err("Process", sys_error("fork()"));
        return h;
    }

    if (pid == 0) {
        execl(exe.c_str(), exe.c_str(), arg1.c_str(), arg2.c_str(), arg3.c_str(), nullptr);
        perror("execl");
        _exit(127);
    }

    h.pid = pid;
    h.is_valid = true;
    log_msg("Process", "Child started (PID: " + std::to_string(pid) + ")");
    return h;
}


ProcessResult wait_process(ProcessHandle& handle) {
    ProcessResult res;
    if (!handle.is_valid) return res;

    int status = 0;
    if (waitpid(handle.pid, &status, 0) < 0) {
        log_err("Process", sys_error("waitpid()"));
        res.exit_code = -1;
    } else {
        res.exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    }
    handle.is_valid = false;
    return res;
}

#pragma once
#include "common/defines.h"

#ifdef OS_LINUX
#include <semaphore.h>
#endif

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

// Размер буфера данных в общей памяти; длинные строки передаются частями
constexpr size_t SHM_DATA_SIZE = 4096;

// Блок, который лежит в отображаемом файле и виден и родителю, и ребёнку
struct SharedBlock {
#ifdef OS_LINUX
    sem_t ready;   // родитель положил порцию данных
    sem_t empty;   // ребёнок забрал порцию, буфер свободен
#endif
    int finished;  // 1 — строк больше не будет
    int last;      // 1 — порция последняя в строке
    size_t len;
    char data[SHM_DATA_SIZE];
};

// Канал между процессами через отображаемый в память файл (memory-mapped file)
class SharedChannel {
public:
    // owner = true — создать файл и семафоры (родитель), false — подключиться (ребёнок)
    SharedChannel(const std::string& path, bool owner);
    ~SharedChannel();

    void write(const std::string& line);   // родитель: передать строку
    void finish();                          // родитель: сообщить, что строк больше нет
    bool read(std::string& line);           // ребёнок: получить строку; false — конец данных

    // родитель: запомнить ребёнка, чтобы не ждать его вечно, если он завершился
    void set_peer(const ProcessHandle& peer) { peer_ = peer; }

    const std::string& path() const { return path_; }

private:
    void wait_empty();                      // ждать, пока ребёнок освободит буфер

    std::string path_;
    bool owner_;
    ProcessHandle peer_;
#ifdef OS_WIN
    HANDLE file_ = nullptr;
    HANDLE mapping_ = nullptr;
#else
    int fd_ = -1;
#endif
    SharedBlock* block_ = nullptr;
};

ProcessHandle start_process(const std::string& exe, const std::string& arg1, const std::string& arg2, const std::string& arg3);
ProcessResult wait_process(ProcessHandle& handle);
std::string remove_vowels(const std::string& str);

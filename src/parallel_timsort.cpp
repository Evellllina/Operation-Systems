#include "parallel_timsort.hpp"
#include "timsort.hpp"

#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <vector>

namespace {

pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

struct SortTask {
    int *a;
    std::size_t lo, hi;
    unsigned hold_sec;
    bool verbose;
};

struct MergeTask {
    int *a;
    std::size_t lo, mid, hi;
};

void *sort_worker(void *arg) {
    auto *t = static_cast<SortTask *>(arg);
    if (t->verbose) {
        pthread_mutex_lock(&print_mutex);
        std::printf("  поток TID=%ld сортирует [%zu, %zu)\n",
                    static_cast<long>(syscall(SYS_gettid)), t->lo, t->hi);
        pthread_mutex_unlock(&print_mutex);
    }
    timsort(t->a + t->lo, t->hi - t->lo);
    if (t->hold_sec) sleep(t->hold_sec);
    return nullptr;
}

void *merge_worker(void *arg) {
    auto *t = static_cast<MergeTask *>(arg);
    std::vector<int> tmp;
    merge_runs(t->a, t->lo, t->mid, t->hi, tmp);
    return nullptr;
}

template <typename Task>
void run_all(std::vector<Task> &tasks, void *(*worker)(void *)) {
    std::vector<pthread_t> tids(tasks.size());
    std::vector<bool> started(tasks.size(), false);

    for (std::size_t i = 0; i < tasks.size(); ++i) {
        int rc = pthread_create(&tids[i], nullptr, worker, &tasks[i]);
        if (rc == 0) {
            started[i] = true;
        } else {
            std::fprintf(stderr, "pthread_create: %s, выполняю в текущем потоке\n",
                         std::strerror(rc));
            worker(&tasks[i]);
        }
    }
    for (std::size_t i = 0; i < tasks.size(); ++i) {
        if (started[i]) pthread_join(tids[i], nullptr);
    }
}

}  

void parallel_timsort(int *a, std::size_t n, int max_threads,
                      unsigned hold_sec, bool verbose) {
    std::size_t parts = static_cast<std::size_t>(max_threads > 0 ? max_threads : 1);
    if (parts > n) parts = n;
    if (parts <= 1) {
        timsort(a, n);
        return;
    }

    std::vector<std::size_t> bounds(parts + 1);
    for (std::size_t i = 0; i <= parts; ++i) bounds[i] = n * i / parts;

    std::vector<SortTask> sort_tasks;
    for (std::size_t i = 0; i < parts; ++i)
        sort_tasks.push_back({a, bounds[i], bounds[i + 1], hold_sec, verbose});
    run_all(sort_tasks, sort_worker);

    while (bounds.size() > 2) {
        std::size_t runs = bounds.size() - 1;
        std::size_t pairs = runs / 2;

        std::vector<MergeTask> merge_tasks;
        std::vector<std::size_t> next{bounds[0]};
        for (std::size_t p = 0; p < pairs; ++p) {
            merge_tasks.push_back({a, bounds[2 * p], bounds[2 * p + 1], bounds[2 * p + 2]});
            next.push_back(bounds[2 * p + 2]);
        }
        if (runs % 2) next.push_back(bounds[runs]); 

        run_all(merge_tasks, merge_worker);
        bounds.swap(next);
    }
}

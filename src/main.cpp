#include "parallel_timsort.hpp"

#include <getopt.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>
#include <vector>

namespace {

void usage(const char *prog) {
    std::fprintf(stderr,
                 "Использование: %s -t THREADS\n"
                 "  -t N     максимум одновременно работающих потоков \n"
                 "  -n N     размер массива (по умолчанию 1000000)\n"
                 "  -g TYPE  данные: random | sorted | reversed | partial (по умолчанию random)\n"
                 "  -s SEED  зерно генератора (по умолчанию 42)\n"
                 "  -c       проверить результат сравнением с std::sort\n"
                 "  -w SEC   задержка в потоках, чтобы увидеть их в ps / top\n"
                 "  -v       печатать PID и TID потоков\n"
                 "  -q       печатать только время в мс (для скрипта замеров)\n",
                 prog);
}

bool parse_number(const char *s, long long &out) {
    char *end = nullptr;
    out = std::strtoll(s, &end, 10);
    return *s != '\0' && *end == '\0';
}

std::vector<int> generate(std::size_t n, const std::string &type, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(-1000000000, 1000000000);
    std::vector<int> v(n);
    for (auto &x : v) x = dist(rng);

    if (type == "sorted") {
        std::sort(v.begin(), v.end());
    } else if (type == "reversed") {
        std::sort(v.begin(), v.end(), [](int a, int b) { return a > b; });
    } else if (type == "partial") {
        std::sort(v.begin(), v.end());
        if (n > 1) {
            std::uniform_int_distribution<std::size_t> pos(0, n - 1);
            for (std::size_t i = 0; i < n / 100; ++i) std::swap(v[pos(rng)], v[pos(rng)]);
        }
    }
    return v;
}

} 

int main(int argc, char *argv[]) {
    long long threads = 0, n = 1000000, seed = 42, hold = 0;
    std::string type = "random";
    bool check = false, verbose = false, quiet = false;

    int opt;
    while ((opt = getopt(argc, argv, "t:n:g:s:w:cvqh")) != -1) {
        bool ok = true;
        switch (opt) {
            case 't': ok = parse_number(optarg, threads) && threads > 0; break;
            case 'n': ok = parse_number(optarg, n) && n >= 0; break;
            case 's': ok = parse_number(optarg, seed); break;
            case 'w': ok = parse_number(optarg, hold) && hold >= 0; break;
            case 'g':
                type = optarg;
                ok = type == "random" || type == "sorted" || type == "reversed" ||
                     type == "partial";
                break;
            case 'c': check = true; break;
            case 'v': verbose = true; break;
            case 'q': quiet = true; break;
            default: usage(argv[0]); return opt == 'h' ? 0 : 1;
        }
        if (!ok) {
            std::fprintf(stderr, "Неверное значение ключа -%c: %s\n", opt, optarg);
            return 1;
        }
    }
    if (threads == 0) {
        std::fprintf(stderr, "Не задано число потоков (-t)\n");
        usage(argv[0]);
        return 1;
    }

    std::vector<int> data = generate(static_cast<std::size_t>(n), type,
                                     static_cast<unsigned>(seed));
    std::vector<int> reference;
    if (check) reference = data;

    if (verbose) {
        std::printf("PID=%d, потоков: %lld\n", getpid(), threads);
        std::fflush(stdout);
    }

    auto start = std::chrono::steady_clock::now();
    parallel_timsort(data.data(), data.size(), static_cast<int>(threads),
                     static_cast<unsigned>(hold), verbose);
    auto finish = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(finish - start).count();

    if (quiet) {
        std::printf("%.3f\n", ms);
    } else {
        std::printf("n=%lld, данные=%s, потоков=%lld, время=%.3f мс\n",
                    n, type.c_str(), threads, ms);
    }

    if (check) {
        std::sort(reference.begin(), reference.end());
        bool ok = data == reference;
        std::printf("Проверка: %s\n", ok ? "OK" : "ОШИБКА");
        if (!ok) return 2;
    }
    return 0;
}

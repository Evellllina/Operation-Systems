#pragma once

#include <cstddef>

// Параллельная TimSort на POSIX-потоках.
// max_threads — сколько потоков работает одновременно (не больше).
// hold_sec    — задержка в каждом потоке после сортировки куска,
//               чтобы успеть увидеть потоки через ps / top.
// verbose     — печатать TID каждого рабочего потока.
void parallel_timsort(int *a, std::size_t n, int max_threads,
                      unsigned hold_sec = 0, bool verbose = false);

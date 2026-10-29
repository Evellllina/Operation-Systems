#pragma once

#include <cstddef>

void parallel_timsort(int *a, std::size_t n, int max_threads,
                      unsigned hold_sec = 0, bool verbose = false);

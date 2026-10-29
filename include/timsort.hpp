#pragma once

#include <cstddef>
#include <vector>

void timsort(int *a, std::size_t n);

void merge_runs(int *a, std::size_t lo, std::size_t mid, std::size_t hi,
                std::vector<int> &tmp);

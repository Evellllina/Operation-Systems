#pragma once

#include <cstddef>
#include <vector>

void timsort(int *a, std::size_t n);

void merge_runs(int *a, std::size_t lo, std::size_t mid, std::size_t hi,
                std::vector<int> &tmp);


// ./lab3 -t 4 -n 100 -v -c

//./lab3 -t 4 -n 1000000 -w 10 -v
//ps -T -p $(pgrep -x lab3)

//./lab3 -t 1 -n 10000000
//./lab3 -t 8 -n 10000000
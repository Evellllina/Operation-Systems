#include "timsort.hpp"

#include <algorithm>
#include <cstring>

namespace {

const std::size_t MIN_MERGE = 64;  
const std::size_t MIN_GALLOP = 7;  
// Ран: где начинается в массиве и какой длины
struct Run {
    std::size_t base;
    std::size_t len;
};
// Считает minrun — минимальную длину рана (от 32 до 64 элементов)
std::size_t compute_minrun(std::size_t n) {
    std::size_t r = 0;
    while (n >= MIN_MERGE) {
        r |= n & 1;
        n >>= 1;
    }
    return n + r;
}
// Находит длину уже упорядоченного куска с позиции lo; убывающий разворачивает
std::size_t count_run(int *a, std::size_t lo, std::size_t hi) {
    std::size_t i = lo + 1;
    if (i == hi) return 1;
    if (a[i] < a[lo]) {
        while (i + 1 < hi && a[i + 1] < a[i]) ++i;
        std::reverse(a + lo, a + i + 1);
    } else {
        while (i + 1 < hi && a[i + 1] >= a[i]) ++i;
    }
    return i + 1 - lo;
}
// Сортировка вставками: каждый элемент ставится на своё место (место ищется бинарным поиском)
void binary_insertion_sort(int *a, std::size_t lo, std::size_t hi, std::size_t start) {
    for (std::size_t i = start; i < hi; ++i) {
        int x = a[i];
        int *pos = std::upper_bound(a + lo, a + i, x);
        std::memmove(pos + 1, pos, (a + i - pos) * sizeof(int));
        *pos = x;
    }
}
// Галоп: быстро считает, сколько элементов <= key (прыжки 1, 2, 4, 8... + бинарный поиск)
std::size_t gallop_right(int key, const int *a, std::size_t n) {
    std::size_t lo = 0, hi = 1;
    while (hi <= n && a[hi - 1] <= key) {
        lo = hi;
        hi *= 2;
    }
    if (hi > n) hi = n;
    return std::upper_bound(a + lo, a + hi, key) - a;
}
// Галоп: быстро считает, сколько элементов < key
std::size_t gallop_left(int key, const int *a, std::size_t n) {
    std::size_t lo = 0, hi = 1;
    while (hi <= n && a[hi - 1] < key) {
        lo = hi;
        hi *= 2;
    }
    if (hi > n) hi = n;
    return std::lower_bound(a + lo, a + hi, key) - a;
}
// Сливает два соседних рана в стеке в один
void merge_at(int *a, std::vector<Run> &stack, std::size_t i, std::vector<int> &tmp) {
    Run &x = stack[i];
    const Run &y = stack[i + 1];
    merge_runs(a, x.base, y.base, y.base + y.len, tmp);
    x.len += y.len;
    stack.erase(stack.begin() + i + 1);
}
// Сливает раны в стеке, если их длины слишком разные (чтобы сливались куски похожей длины)
void merge_collapse(int *a, std::vector<Run> &stack, std::vector<int> &tmp) {
    while (stack.size() > 1) {
        std::size_t n = stack.size() - 2;
        if ((n > 0 && stack[n - 1].len <= stack[n].len + stack[n + 1].len) ||
            (n > 1 && stack[n - 2].len <= stack[n - 1].len + stack[n].len)) {
            if (stack[n - 1].len < stack[n + 1].len) --n;
            merge_at(a, stack, n, tmp);
        } else if (stack[n].len <= stack[n + 1].len) {
            merge_at(a, stack, n, tmp);
        } else {
            break;
        }
    }
}
// В конце сливает все оставшиеся в стеке раны в один
void merge_force_collapse(int *a, std::vector<Run> &stack, std::vector<int> &tmp) {
    while (stack.size() > 1) {
        std::size_t n = stack.size() - 2;
        if (n > 0 && stack[n - 1].len < stack[n + 1].len) --n;
        merge_at(a, stack, n, tmp);
    }
}

}  
// Слияние двух соседних отсортированных кусков
void merge_runs(int *a, std::size_t lo, std::size_t mid, std::size_t hi,
                std::vector<int> &tmp) {
    if (lo == mid || mid == hi) return;

    lo += gallop_right(a[mid], a + lo, mid - lo);
    if (lo == mid) return;
    hi = mid + gallop_left(a[mid - 1], a + mid, hi - mid);
    if (hi == mid) return;

    std::size_t na = mid - lo;
    tmp.assign(a + lo, a + mid);
    const int *left = tmp.data();

    std::size_t i = 0, j = mid, k = lo;
    std::size_t wins_left = 0, wins_right = 0;
    while (i < na && j < hi) {
        if (a[j] < left[i]) {
            a[k++] = a[j++];
            ++wins_right;
            wins_left = 0;
            if (wins_right >= MIN_GALLOP) {
                std::size_t cnt = gallop_left(left[i], a + j, hi - j);
                std::memmove(a + k, a + j, cnt * sizeof(int));
                k += cnt;
                j += cnt;
                wins_right = 0;
            }
        } else {
            a[k++] = left[i++];
            ++wins_left;
            wins_right = 0;
            if (wins_left >= MIN_GALLOP && j < hi) {
                std::size_t cnt = gallop_right(a[j], left + i, na - i);
                std::memcpy(a + k, left + i, cnt * sizeof(int));
                k += cnt;
                i += cnt;
                wins_left = 0;
            }
        }
    }
    std::memcpy(a + k, left + i, (na - i) * sizeof(int));
}

void timsort(int *a, std::size_t n) {
    if (n < 2) return; 

    std::vector<int> tmp;    //буфер для слияний
    std::vector<Run> stack;
    std::size_t minrun = compute_minrun(n);  //мин длина рана

    std::size_t lo = 0;
    while (lo < n) {
        //Длина уже упорядоченного куска
        std::size_t len = count_run(a, lo, n);

        if (len < minrun) {
            std::size_t forced = std::min(minrun, n - lo);  //дополняем его до minrun вставками
            binary_insertion_sort(a, lo, lo + forced, lo + len);
            len = forced;
        }

        //Кладём ран в стек и сливаем с соседями, если нарушен баланс длин
        stack.push_back({lo, len});
        merge_collapse(a, stack, tmp);

        lo += len; 
    }

    merge_force_collapse(a, stack, tmp);
}
#!/usr/bin/env bash
# Замеры ускорения S = T1 / Tn и эффективности E = S / n.
# Каждый запуск повторяется REPEAT раз, берётся лучшее время.
# Запуск из папки lab3:  ./scripts/bench.sh [файл.csv]
set -euo pipefail

BIN=./lab3
OUT=${1:-results/bench.csv}
SIZES="100000 1000000 10000000"
THREADS="1 2 4 8 16"
DATA="random partial reversed"
REPEAT=3

best_time() {  # $1 — тип данных, $2 — размер, $3 — потоки
    local best="" t
    for _ in $(seq "$REPEAT"); do
        t=$("$BIN" -q -g "$1" -n "$2" -t "$3")
        if [[ -z $best ]] || awk -v a="$t" -v b="$best" 'BEGIN { exit !(a < b) }'; then
            best=$t
        fi
    done
    echo "$best"
}

mkdir -p "$(dirname "$OUT")"
echo "data,n,threads,time_ms,speedup,efficiency" > "$OUT"

for d in $DATA; do
    for n in $SIZES; do
        t1=$(best_time "$d" "$n" 1)
        for t in $THREADS; do
            if [[ $t -eq 1 ]]; then tn=$t1; else tn=$(best_time "$d" "$n" "$t"); fi
            awk -v d="$d" -v n="$n" -v t="$t" -v t1="$t1" -v tn="$tn" \
                'BEGIN { s = t1 / tn; printf "%s,%s,%s,%.3f,%.3f,%.3f\n", d, n, t, tn, s, s / t }' >> "$OUT"
        done
    done
done

column -s, -t "$OUT"

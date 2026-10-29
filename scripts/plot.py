#!/usr/bin/env python3
# Графики ускорения и эффективности по results/bench.csv.
# Запуск из папки lab3:  python3 scripts/plot.py
# Нужен matplotlib (pip install matplotlib).
import csv
import sys
from collections import defaultdict

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

src = sys.argv[1] if len(sys.argv) > 1 else "results/bench.csv"
out_dir = sys.argv[2] if len(sys.argv) > 2 else "report"

rows = defaultdict(list)  # (data, n) -> [(threads, speedup, efficiency)]
with open(src) as f:
    for r in csv.DictReader(f):
        rows[(r["data"], int(r["n"]))].append(
            (int(r["threads"]), float(r["speedup"]), float(r["efficiency"]))
        )

datasets = sorted({d for d, _ in rows}, key=["random", "partial", "reversed", "sorted"].index)
titles = {"random": "случайные", "partial": "почти отсортированные",
          "reversed": "обратный порядок", "sorted": "отсортированные"}


def plot(metric_idx, ylabel, filename, ideal):
    fig, axes = plt.subplots(1, len(datasets), figsize=(4.2 * len(datasets), 3.6), sharey=True)
    if len(datasets) == 1:
        axes = [axes]
    for ax, d in zip(axes, datasets):
        threads = []
        for (dd, n), pts in sorted(rows.items()):
            if dd != d:
                continue
            pts.sort()
            threads = [p[0] for p in pts]
            ax.plot(threads, [p[metric_idx] for p in pts], marker="o", label=f"n = {n:,}".replace(",", " "))
        ax.plot(threads, [ideal(t) for t in threads], "k--", linewidth=1, label="идеал")
        ax.set_xscale("log", base=2)
        ax.set_xticks(threads)
        ax.set_xticklabels(threads)
        ax.set_title(titles.get(d, d))
        ax.set_xlabel("число потоков")
        ax.grid(alpha=0.3)
    axes[0].set_ylabel(ylabel)
    axes[0].legend(fontsize=8)
    fig.tight_layout()
    fig.savefig(f"{out_dir}/{filename}", dpi=200)


plot(1, "ускорение S", "speedup.png", lambda t: t)
plot(2, "эффективность E", "efficiency.png", lambda t: 1)

# Таблица замеров для отчёта (подключается через \input)
with open(src) as f, open(f"{out_dir}/bench_table.tex", "w") as out:
    prev = None
    for r in csv.DictReader(f):
        key = (r["data"], r["n"])
        if prev is not None and key[0] != prev[0]:
            out.write("\\hline\n")
        elif prev is not None and key != prev:
            out.write("\\cline{2-6}\n")
        n = f"{int(r['n']):,}".replace(",", "\\,")
        out.write(f"{r['data']} & {n} & {r['threads']} & {float(r['time_ms']):.1f} & "
                  f"{float(r['speedup']):.2f} & {float(r['efficiency']):.2f} \\\\\n")
        prev = key

#!/usr/bin/env python3
"""Draw all Part 2 plots from the CSVs in part2/results (run anywhere with matplotlib + pandas)."""
import os

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import pandas as pd

from common_io import RESULTS


def save(fig, name):
    fig.tight_layout()
    p = os.path.join(RESULTS, name)
    fig.savefig(p, dpi=150)
    print("wrote", p)


def line(x, y, xlabel, title, name, logx=False):
    fig, ax = plt.subplots(figsize=(6, 4))
    y = [v / 1e6 for v in y]
    ax.plot(x, y, marker="o")
    if logx:
        ax.set_xscale("log", base=2)
        ax.set_xticks(list(x))
        ax.set_xticklabels([str(v) for v in x])
    ax.set_xlabel(xlabel)
    ax.set_ylabel("system.cpu.numCycles (millions)")
    ax.grid(alpha=.3)
    for xi, yi in zip(x, y):
        ax.annotate(f"{yi:.2f}", (xi, yi), textcoords="offset points", xytext=(0, 6),
                    ha="center", fontsize=7)
    save(fig, name)


uf = pd.read_csv(os.path.join(RESULTS, "q3_unrolling.csv"))
line(uf.unrolling_factor, uf.numCycles, "unrolling factor (1 = tables.exe)",
     "Q3: cycles vs unrolling factor (L1I=L1D=8kB, L2=128kB)", "q3_unrolling.png", logx=True)

l1d = pd.read_csv(os.path.join(RESULTS, "q3_l1d.csv"))
line(l1d.l1d_kB, l1d.numCycles, "L1D size (kB)",
     "Q3: cycles vs L1D size (tables.exe, L1I=8kB, L2=128kB)", "q3_l1d.png", logx=True)

ex_path = os.path.join(RESULTS, "q4_exhaustive.csv")
if os.path.exists(ex_path):
    ex = pd.read_csv(ex_path)
    po = pd.read_csv(os.path.join(RESULTS, "q4_pareto.csv")).sort_values("totalMemoryKB")
    fig, ax = plt.subplots(figsize=(7, 4.5))
    ax.scatter(ex.totalMemoryKB, ex.latencyCC / 1e6, s=8, alpha=.35, label="all 720 configurations")
    ax.plot(po.totalMemoryKB, po.latencyCC / 1e6, "r-o", ms=4, label="Pareto front (exhaustive)")
    ga_path = os.path.join(RESULTS, "q5_genOptimizer_PO.csv")
    if os.path.exists(ga_path):
        ga = pd.read_csv(ga_path).sort_values("total_mem_kB")
        ax.plot(ga.total_mem_kB, ga.exec_time_cc / 1e6, "g--s", ms=5, mfc="none", label="Pareto front (NSGA-II)")
    ax.set_xscale("log", base=2)
    ax.set_xlabel("total memory L1I+L1D+L2 (kB)")
    ax.set_ylabel("latency (million cycles)")
    ax.grid(alpha=.3)
    ax.legend()
    save(fig, "q4_q5_pareto.png")

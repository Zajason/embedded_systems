#!/usr/bin/env python3
"""Q3: (a) unrolling factor sweep, (b) L1D size sweep, on the two-level system."""
import os
from concurrent.futures import ProcessPoolExecutor

import gem5sim as g
from common_io import write_csv

WORKERS = int(os.environ.get("WORKERS", "6"))


def uf_job(uf):
    c, t = g.run(g.TWO_LEVEL, g.exe(uf), "8kB", "8kB", "128kB")
    return uf or 1, c, t


def l1d_job(size):
    c, t = g.run(g.TWO_LEVEL, g.exe(), "8kB", size, "128kB")
    return size, c, t


if __name__ == "__main__":
    with ProcessPoolExecutor(WORKERS) as ex:
        ufs = list(ex.map(uf_job, [None] + g.UFS))      # None = tables.exe (no unrolling, UF=1)
        l1d = list(ex.map(l1d_job, g.L1_SIZES))
    for uf, c, _ in ufs:
        print(f"UF={uf:>2}: {c}")
    for s, c, _ in l1d:
        print(f"L1D={s:>5}: {c}")
    write_csv("q3_unrolling.csv", ["unrolling_factor", "numCycles", "sim_wall_s"],
              [[u, c, round(t, 1)] for u, c, t in ufs])
    write_csv("q3_l1d.csv", ["l1d_size", "l1d_kB", "numCycles", "sim_wall_s"],
              [[s, g.kb(s), c, round(t, 1)] for s, c, t in l1d])

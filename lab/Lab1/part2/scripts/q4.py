#!/usr/bin/env python3
"""
Q4: exhaustive DSE over L1I x L1D x L2 x UF (6*6*4*5 = 720 simulations),
then the Pareto front (min latencyCC, min totalMemoryKB) with paretoset.

Resumable: finished configurations are kept in results/q4_exhaustive.csv and
skipped if the script is started again.
"""
import csv
import itertools
import os
import time
from concurrent.futures import ProcessPoolExecutor, as_completed

import gem5sim as g
from common_io import RESULTS, write_csv

WORKERS = int(os.environ.get("WORKERS", "6"))
CSV = os.path.join(RESULTS, "q4_exhaustive.csv")
HEADER = ["L1I_cache_size", "L1D_cache_size", "L2_cache_size", "unrolling_factor",
          "totalMemoryKB", "latencyCC", "sim_wall_s"]


def job(cfg):
    l1i, l1d, l2, uf = cfg
    c, t = g.run(g.TWO_LEVEL, g.exe(uf), l1i, l1d, l2)
    return [l1i, l1d, l2, uf, g.kb(l1i) + g.kb(l1d) + g.kb(l2), c, round(t, 1)]


def main():
    space = list(itertools.product(g.L1_SIZES, g.L1_SIZES, g.L2_SIZES, g.UFS))
    done = {}
    if os.path.exists(CSV):
        with open(CSV) as f:
            for r in csv.DictReader(f):
                done[(r["L1I_cache_size"], r["L1D_cache_size"], r["L2_cache_size"],
                      int(r["unrolling_factor"]))] = r
    todo = [c for c in space if c not in done]
    print(f"{len(space)} configurations, {len(done)} already done, {len(todo)} to run, {WORKERS} workers")

    new_file = not os.path.exists(CSV)
    t0 = time.time()
    with open(CSV, "a", newline="") as f, ProcessPoolExecutor(WORKERS) as ex:
        w = csv.writer(f)
        if new_file:
            w.writerow(HEADER)
        futs = [ex.submit(job, c) for c in todo]
        for i, fut in enumerate(as_completed(futs), 1):
            w.writerow(fut.result())
            f.flush()
            if i % 10 == 0 or i == len(todo):
                el = time.time() - t0
                print(f"  {i}/{len(todo)} done, {el/60:.1f} min elapsed, "
                      f"~{el/i*(len(todo)-i)/60:.0f} min left", flush=True)
    wall = time.time() - t0
    with open(os.path.join(RESULTS, "q4_runtime.txt"), "a") as f:
        f.write(f"ran {len(todo)} simulations in {wall:.0f} s wall time with {WORKERS} workers\n")
    pareto()


def pareto():
    import pandas as pd
    from paretoset import paretoset

    df = pd.read_csv(CSV)
    data = df[["totalMemoryKB", "latencyCC"]]
    mask = paretoset(data, sense=["min", "min"], distinct=False)
    po = df[mask].sort_values(["totalMemoryKB", "latencyCC"])
    po.to_csv(os.path.join(RESULTS, "q4_pareto.csv"), index=False)
    total_sim = df["sim_wall_s"].sum()
    print(f"\nPareto-optimal configurations ({len(po)}):")
    print(po.to_string(index=False))
    print(f"\nsum of single-simulation times: {total_sim/60:.1f} min (= sequential exhaustive search)")
    print("wrote", os.path.join(RESULTS, "q4_pareto.csv"))


if __name__ == "__main__":
    main()

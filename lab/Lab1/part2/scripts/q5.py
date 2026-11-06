#!/usr/bin/env python3
"""
Q5: run the course's genOptimizer.py (NSGA-II, pymoo) unchanged, time it and
count how many gem5 evaluations it did. It must run from /gem5 (it uses
./m5out and imports pmodules/).
"""
import os
import re
import shutil
import subprocess
import time

from common_io import RESULTS

GEM5 = os.environ.get("GEM5_DIR", "/gem5")
t0 = time.time()
p = subprocess.run(["python3", "genOptimizer.py"], cwd=GEM5,
                   stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
wall = time.time() - t0
log = p.stdout
open(os.path.join(RESULTS, "q5_genOptimizer.log"), "w").write(log)

# pymoo's verbose table: "|  n_gen  |  n_eval  | ..." rows; gem5's own output is mixed in
evals = [int(m.group(2)) for m in re.finditer(r"^\s*(\d+)\s*\|\s*(\d+)\s*\|", log, re.M)]
n_eval = evals[-1] if evals else None
shutil.copy(os.path.join(GEM5, "genOptimizer_PO.csv"), os.path.join(RESULTS, "q5_genOptimizer_PO.csv"))
summary = f"genOptimizer: {n_eval} evaluations, {wall:.0f} s wall time (exit code {p.returncode})\n"
open(os.path.join(RESULTS, "q5_runtime.txt"), "w").write(summary)
print(log[-3000:])
print(summary)

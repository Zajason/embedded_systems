"""
gem5sim.py - shared helpers for all Part 2 scripts (run INSIDE the gem5 container).

run(config, exe, l1i=None, l1d=None, l2=None) runs one gem5 simulation in its own
output directory (so several can run in parallel) and returns
system.cpu.numCycles from that run's stats.txt.
"""
import os
import re
import subprocess
import time

GEM5 = os.environ.get("GEM5_DIR", "/gem5")
GEM5_BIN = os.path.join(GEM5, "build/X86/gem5.opt")
SIMPLE = os.path.join(GEM5, "configs/learning_gem5/part1/simple.py")
TWO_LEVEL = os.path.join(GEM5, "configs/learning_gem5/part1/two_level.py")
TABLES = os.path.join(GEM5, "tables_UF")
OUT_ROOT = os.environ.get("SIM_OUT", "/tmp/gem5runs")

L1_SIZES = ["2kB", "4kB", "8kB", "16kB", "32kB", "64kB"]
L2_SIZES = ["128kB", "256kB", "512kB", "1024kB"]
UFS = [2, 4, 8, 16, 32]


def exe(uf=None):
    """tables.exe (no unrolling) or tables_uf<uf>.exe - absolute path."""
    name = "tables.exe" if uf is None else f"tables_uf{uf}.exe"
    return os.path.join(TABLES, name)


def kb(size):
    return int(size.lower().replace("kb", ""))


def num_cycles(stats_file):
    with open(stats_file) as f:
        for line in f:
            if line.startswith("system.cpu.numCycles"):
                return int(float(line.split()[1]))
    raise RuntimeError("system.cpu.numCycles not found in " + stats_file)


def run(config, binary, l1i=None, l1d=None, l2=None, tag=None):
    """Run one simulation, return (cycles, wall_seconds)."""
    tag = tag or "_".join(str(v) for v in
                          (os.path.basename(config)[:-3], os.path.basename(binary)[:-4], l1i, l1d, l2)
                          if v is not None)
    outdir = os.path.join(OUT_ROOT, tag)
    os.makedirs(outdir, exist_ok=True)
    cmd = [GEM5_BIN, "--outdir=" + outdir, config, binary]
    if l1i: cmd.append("--l1i_size=" + l1i)
    if l1d: cmd.append("--l1d_size=" + l1d)
    if l2:  cmd.append("--l2_size=" + l2)
    t0 = time.time()
    with open(os.path.join(outdir, "gem5.log"), "w") as log:
        res = subprocess.run(cmd, stdout=log, stderr=subprocess.STDOUT, cwd=GEM5)
    wall = time.time() - t0
    if res.returncode != 0:
        raise RuntimeError(f"gem5 failed ({res.returncode}): {' '.join(cmd)} - see {outdir}/gem5.log")
    return num_cycles(os.path.join(outdir, "stats.txt")), wall

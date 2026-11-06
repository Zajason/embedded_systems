#!/usr/bin/env python3
"""Q1 + Q2: tables.exe on the simple (no cache) and the two-level cache system."""
import gem5sim as g
from common_io import write_csv

rows = []
c, t = g.run(g.SIMPLE, g.exe())
print(f"Q1 simple.py (CPU + memory, no caches): numCycles = {c}   ({t:.0f}s)")
rows.append(["simple (no caches)", "-", "-", "-", c, round(t, 1)])

c, t = g.run(g.TWO_LEVEL, g.exe(), "8kB", "8kB", "128kB")
print(f"Q2 two_level.py L1I=8kB L1D=8kB L2=128kB: numCycles = {c}   ({t:.0f}s)")
rows.append(["two_level", "8kB", "8kB", "128kB", c, round(t, 1)])

write_csv("q1_q2.csv", ["system", "l1i", "l1d", "l2", "numCycles", "sim_wall_s"], rows)

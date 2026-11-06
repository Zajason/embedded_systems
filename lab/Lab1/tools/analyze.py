#!/usr/bin/env python3
"""
Turn the GDB dump of g_results (pasted from e2studio's Debugger Console)
into a CSV and the line plots for Q2-Q5.

On the board, after the program stops at the BKPT, type in the Debugger Console:
    set print elements 0
    set print pretty off
    p g_core_clock_hz
    p g_all_correct
    p g_results
Copy everything it prints into results/gdb_dump.txt, then (on any PC):
    python3 tools/analyze.py results/gdb_dump.txt
Outputs results/results.csv and results/*.png.
Needs matplotlib only for the plots (pip install matplotlib).
"""
import csv
import re
import sys
from pathlib import Path

FIELDS = ["question", "bx", "by", "min_cycles", "max_cycles", "avg_cycles",
          "min_us", "max_us", "avg_us", "correct"]


def parse(text):
    rows = []
    # every struct starts with "name = 0x... \"..."
    chunks = re.split(r"\{\s*name\s*=", text)[1:]
    for ch in chunks:
        m = re.search(r'"([^"]+)"', ch)
        row = {"name": m.group(1) if m else "?"}
        body = re.sub(r"cycles\s*=\s*\{[^}]*\}", "", ch)  # drop per-run array
        for f in FIELDS:
            mm = re.search(r"\b" + f + r"\s*=\s*(-?[0-9.]+(?:e[-+]?\d+)?)", body)
            row[f] = float(mm.group(1)) if mm else None
        cyc = re.search(r"cycles\s*=\s*\{([^}]*)\}", ch)
        row["cycles"] = cyc.group(1).strip() if cyc else ""
        rows.append(row)
    clk = re.search(r"\$\d+\s*=\s*(\d{6,})", text)  # p g_core_clock_hz
    return rows, (int(clk.group(1)) if clk else None)


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    src = Path(sys.argv[1])
    rows, clk = parse(src.read_text(errors="replace"))
    if not rows:
        sys.exit("no g_results entries found in " + str(src))
    out = src.parent
    with open(out / "results.csv", "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["name"] + FIELDS)
        for r in rows:
            w.writerow([r["name"]] + [r[k] for k in FIELDS])
    print(f"core clock: {clk} Hz" if clk else "core clock: (not found)")
    print(f"{'name':22s} {'BxBy':>6s} {'avg_us':>9s} {'min_us':>9s} {'max_us':>9s}  ok")
    for r in rows:
        print(f"{r['name']:22s} {int(r['bx']):>2d}x{int(r['by']):<3d} "
              f"{r['avg_us']:9.2f} {r['min_us']:9.2f} {r['max_us']:9.2f}  {int(r['correct'])}")
    print("wrote", out / "results.csv")

    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        print("matplotlib not installed: skipping plots")
        return

    def lineplot(sel, labels, title, fname):
        fig, ax = plt.subplots(figsize=(max(6, len(sel) * 0.6), 4))
        xs = range(len(sel))
        ax.plot(xs, [r["avg_us"] for r in sel], marker="o", label="avg")
        ax.plot(xs, [r["min_us"] for r in sel], marker="v", ls="--", label="min")
        ax.plot(xs, [r["max_us"] for r in sel], marker="^", ls="--", label="max")
        ax.set_xticks(list(xs))
        ax.set_xticklabels(labels, rotation=45, ha="right")
        ax.set_ylabel("execution time (us)")
        ax.set_title(title)
        ax.grid(alpha=.3)
        ax.legend()
        fig.tight_layout()
        fig.savefig(out / fname, dpi=150)
        print("wrote", out / fname)

    q23 = [r for r in rows if r["question"] in (2, 3)]
    lineplot(q23, [r["name"].replace("phods_", "") for r in q23],
             "Q2-Q3: original vs. each transformation (B=5)", "q2_q3.png")
    q4 = [r for r in rows if r["question"] == 5 and r["bx"] == r["by"]]
    lineplot(q4, [f"B={int(r['bx'])}" for r in q4], "Q4: square block size", "q4.png")
    q5 = [r for r in rows if r["question"] == 5]
    lineplot(q5, [f"{int(r['bx'])}x{int(r['by'])}" for r in q5],
             "Q5: rectangular blocks Bx x By", "q5.png")
    combo = q23 + q5
    lineplot(combo, [r["name"].replace("phods_", "") for r in combo],
             "Q2-Q5: all configurations", "q2_q5_all.png")


if __name__ == "__main__":
    main()

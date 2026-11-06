#!/usr/bin/env python3
"""
Reference board timings from a PAST student's public report, for comparing at the lab.
NOT our measurements - they only go into report_reference.pdf, printed in blue.

Source: github.com/jimmichalako/embedded-systems-ntua, lab-1/report.pdf (2024-25),
same exercise, same skeleton (N = M = 10), SK-S7G2 at 240 MHz.
Their optimized code differs from ours, so only the original (v0) and the
B / Bx x By exploration are comparable; v1-v4 stay empty.

Writes report/generated/reference_values.tex and report/generated/reference_q2_q5.png.
"""
from pathlib import Path

CLOCK = 240_000_000
SOURCE = "jimmichalako/embedded-systems-ntua (2024--25)"

# (min, avg, max) in ms, copied from their report
ORIGINAL = (1.82112085, 1.8211212, 1.82112504)
BEST_OPT_B5 = (0.737999973, 0.738000381, 0.738004164)   # their final optimized code, B = 5
DSE = {
    (1, 1): (1.23679172, 1.23679242, 1.23679999),
    (1, 2): (0.931037473, 0.931038754, 0.931049988),
    (1, 5): (0.769116683, 0.769117032, 0.769120816),
    (1, 10): (0.719620846, 0.719621661, 0.71962917),
    (2, 1): (1.00595411, 1.00595539, 1.00596668),
    (2, 2): (0.848137483, 0.848138356, 0.848145806),
    (2, 5): (0.737766677, 0.737767492, 0.737775001),
    (2, 10): (0.684941653, 0.684942876, 0.684954168),
    (5, 1): (0.864458329, 0.864458736, 0.86446252),
    (5, 2): (0.773754145, 0.773755019, 0.773762527),
    (5, 5): (0.738008355, 0.738009578, 0.738020812),
    (5, 10): (0.70006249, 0.700062898, 0.700066681),
    (10, 1): (0.834158331, 0.834159204, 0.834166654),
    (10, 2): (0.747362501, 0.747363723, 0.747375016),
    (10, 5): (0.711379165, 0.711379573, 0.711383356),
    (10, 10): (0.681566657, 0.681567879, 0.681579171),
}


def us(ms):
    return f"{ms * 1000:.2f}".replace(".", ",")


def cyc(ms):
    return f"{round(ms / 1000 * CLOCK):,}".replace(",", "\\,")


def main():
    gen = Path(__file__).resolve().parents[1] / "report" / "generated"
    gen.mkdir(parents=True, exist_ok=True)
    d = {"clock-mhz": "240", "clock-hz": "240\\,000\\,000", "ref-source": SOURCE,
         "ref-bestopt-us": us(BEST_OPT_B5[1])}
    rows = {"v0": ORIGINAL, **{f"d{bx}-{by}": t for (bx, by), t in DSE.items()}}
    for k, (mn, av, mx) in rows.items():
        d[f"{k}-min-us"], d[f"{k}-avg-us"], d[f"{k}-max-us"] = us(mn), us(av), us(mx)
        d[f"{k}-min-cyc"], d[f"{k}-avg-cyc"], d[f"{k}-max-cyc"] = cyc(mn), cyc(av), cyc(mx)
        d[f"{k}-speedup"] = f"{ORIGINAL[1] / av:.2f}".replace(".", ",")
    sq = min((b for b in DSE if b[0] == b[1]), key=lambda b: DSE[b][1])
    best = min(DSE, key=lambda b: DSE[b][1])
    d["best-b"], d["best-b-us"] = str(sq[0]), us(DSE[sq][1])
    d["best-bx"], d["best-by"], d["best-bxby-us"] = str(best[0]), str(best[1]), us(DSE[best][1])

    with open(gen / "reference_values.tex", "w") as f:
        f.write("% REFERENCE values from a past student's report (tools/make_reference.py) - NOT our measurements\n")
        for k, v in sorted(d.items()):
            f.write(f"\\expandafter\\def\\csname val@{k}\\endcsname{{\\textcolor{{refblue}}{{{v}}}}}\n")
    print("wrote", gen / "reference_values.tex")

    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    labels = ["v0_original"] + [f"dse_{bx}_{by}" for bx, by in DSE]
    vals = [ORIGINAL] + list(DSE.values())
    xs = range(len(vals))
    fig, ax = plt.subplots(figsize=(10, 4))
    ax.plot(xs, [v[1] * 1000 for v in vals], marker="o", label="avg")
    ax.plot(xs, [v[0] * 1000 for v in vals], marker="v", ls="--", label="min")
    ax.plot(xs, [v[2] * 1000 for v in vals], marker="^", ls="--", label="max")
    ax.set_xticks(list(xs))
    ax.set_xticklabels(labels, rotation=45, ha="right")
    ax.set_ylabel("execution time (us)")
    ax.grid(alpha=.3)
    ax.legend()
    ax.text(0.99, 0.95, "REFERENCE (past student's data)", transform=ax.transAxes,
            ha="right", va="top", color="tab:blue", fontsize=9)
    fig.tight_layout()
    fig.savefig(gen / "reference_q2_q5.png", dpi=150)
    print("wrote", gen / "reference_q2_q5.png")


if __name__ == "__main__":
    main()

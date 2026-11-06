# Embedded Systems Lab 1

| doc | what |
|---|---|
| [LAB_GUIDE.md](LAB_GUIDE.md) | step-by-step for the lab PC (Part 1 on the board) and bringing the results home |
| [ANSWERS.md](ANSWERS.md) | report draft: Part 1 (Q1 filled, Q2–Q5 after the lab) and Part 2 (all done, with results) |
| [EXPLANATIONS.md](EXPLANATIONS.md) | detailed explanation of all the code, both parts |
| [part2/README.md](part2/README.md) | how to (re-)run the gem5 part |

## Part 1: PHODS on the Renesas S7G2

The code for **Ζητούμενο 1** (Q2–Q5) is ready. You flash the board once and one debug
session times every configuration. Each one runs 10 times and you get min/max/avg.

```
src/                 -> copy ALL of these into the e2studio project's src/ folder
  hal_entry.c          replaces the Blinky hal_entry.c (runs everything, stores results)
  images.h             frames (given)
  phods_config.h       N, M, B, number of runs
  phods_baseline.h     Q2  original algorithm (unchanged)
  phods_variants.h     Q3  v1: loop-invariant code motion
  phods_impl.h         Q3+ optimized "template" (v2, v3, v4 and all DSE configs)
  phods_instances.h    instantiates v2/v3/v4 and the 16 (Bx,By) DSE functions
  phods_runs.h         table of all configurations + DWT timing wrapper
reference/           original skeleton from the course (do NOT copy into the project)
test/                PC-only correctness test (make -C test run)
tools/analyze.py     GDB dump -> results.csv + line plots
results/             put your dump/CSV/plots here
```

## On the lab PC

1. `git clone <this repo>`.
2. e2studio: **File → New → Renesas C/C++ Project → Renesas Synergy → Synergy C
   Executable Project**, board **S7G2 SK**, template **Blinky** (see the starter guide).
3. Delete the generated `src/hal_entry.c`. Copy everything from this repo's `src/` into
   the project's `src/` (drag and drop in the Project Explorer and pick *Copy files*).
4. Set **-O0**: right click the project → Properties → C/C++ Build → Settings →
   Optimization → *Other optimization flags* = `-O0` (or set Optimization level to None).
   Setting it only on hal_entry.c (as the starter guide does) is also enough, because all
   the PHODS code is in headers that hal_entry.c includes.
5. **Build**, then **Debug As → Renesas GDB Hardware Debugging**, then Resume (F8).
   It halts by itself at `__BKPT(0)` near the end of `hal_entry()`. Without a debugger
   attached that instruction would fault, which is fine here.
6. Read the results. Either:
   - **Expressions view**: add `g_core_clock_hz`, `g_all_correct`, `g_results`; or
   - **Debugger Console** (easiest to save), type:
     ```
     set print elements 0
     set print pretty off
     p g_core_clock_hz
     p g_all_correct
     p g_results
     ```
     Copy the output into `results/gdb_dump.txt` and commit/push it.
7. Anywhere (e.g. back on the Mac):
   ```bash
   pip install matplotlib
   python3 tools/analyze.py results/gdb_dump.txt
   ```
   This writes `results/results.csv` and `q2_q3.png`, `q4.png`, `q5.png`, `q2_q5_all.png`.

`g_all_correct` must be **1**. It means every 5×5 variant gave the same motion vectors as
the original on the board.

## What each configuration is (for the report)

| row in `g_results` | question | change |
|---|---|---|
| `phods_v0_original` | Q2 | original code |
| `phods_v1_hoist` | Q3 | loop-invariant code motion: `B*x`, `B*y` once per block; `vectors_x/y[x][y]` kept in locals; row bound check moved out of the `l` loop; separate init loop removed |
| `phods_v2_bounds` | Q3 | + bounds-check elimination: valid row/column range of each candidate computed once, so the inner loop has no `if`; out-of-frame rows use a precomputed Σ\|cur\| per row |
| `phods_v3_share` | Q3 | + data reuse: the `i = 0` candidate is the same block in the X and the Y search, so it is computed once (5 SADs per step instead of 6) |
| `phods_v4_copy` | Q3 | + copy the current block from `const` data (Flash) into a local buffer (RAM) once per block. See the Hint in the exercise: only `const` data lives in Flash, so measure whether this helps or not and explain why |
| `phods_dse_<b>_<b>` | Q4 | square blocks B ∈ {1, 2, 5, 10} (common divisors of N = M = 10) |
| `phods_dse_<bx>_<by>` | Q5 | all 16 pairs Bx \| N, By \| M |

Q4/Q5 use the v3 code. If v4 turns out faster on the board, change
`PHODS_DSE_COPY_BLOCK` to `1` in `src/phods_instances.h` and re-run, so the DSE uses the
best Q3 code.

Q1 (RAM/Flash sizes, caches) comes from the S7G2 datasheet. The clock frequency is
`g_core_clock_hz` (= `SystemCoreClock`) or the Clocks tab of the configurator.

## Notes

- `N = M = 10` as in the given skeleton (`images.h` has more data; only the first 100
  values are used). With these frames most motion vectors come out 0, so the on-board
  check is weak. `test/host_test.c` also checks every variant against the original on
  2000 random frame pairs (all pass).
- Each measurement is only the call of the PHODS function (`DWT->CYCCNT` read right
  before and after it). Time = cycles / `SystemCoreClock`.
- Expect the 10 runs to be almost identical. Past students' min and max differ by only
  nanoseconds to a few µs, because nothing else runs and the inputs never change. If the
  first run is noticeably slower (cold flash cache), mention it in the report.
- Expected values from past years (≈ 1.82–1.85 ms for the original, B = 10 fastest) are
  in [LAB_GUIDE.md](LAB_GUIDE.md), step "7b-check".
- The unused `#define p 7` from the skeleton is dropped on purpose: a macro named `p`
  breaks other code.

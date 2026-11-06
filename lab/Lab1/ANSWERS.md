# Lab 1 – Ζητούμενο 1: answers / report draft

Status: Q1 is filled in from the datasheet. Q2–Q5 have the code ready. Their numbers come
from the board run (`results/gdb_dump.txt` → `python3 tools/analyze.py`).

---

## Q1 – Board characteristics (Renesas S7G2, part R7FS7G27H3A01CFC on the SK-S7G2)

Source: *S7G2 Microcontroller Group Datasheet*, R01DS0262EU0150 Rev.1.50 (Sep 2025),
features page, Table 1.1 (Arm core), Table 1.2 (Memory), product table on p.6 and §2 (clock timing).

### Memories

| Memory | Size | Notes (datasheet) |
|---|---|---|
| Code flash | **4 MB** | "80 MHz zero wait states". Holds the program **and all `const` data**, so `current[]`/`previous[]` from `images.h` live here |
| Data flash | **64 KB** | for non-volatile user data, 125,000 erase/write cycles |
| SRAM | **640 KB** | high-speed on-chip SRAM: the first 32 KB of SRAM0 have Double-bit Error Detection (DED), the other 608 KB have parity |
| Standby SRAM | **8 KB** | keeps its contents in Deep Software Standby mode |

### Caches and their function

- **Flash Cache (FCACHE)**: the datasheet lists it under Memory. It sits in front of
  the code flash. The flash itself has zero wait states only up to 80 MHz, but the CPU
  runs at up to 240 MHz, so every flash access that misses costs extra wait cycles.
  FCACHE keeps recently read flash contents, both instructions and the `const` data
  (here the image arrays), so repeated reads hit the cache instead of paying the wait
  states. Details are in the User's Manual, flash/FCACHE section.
- **The CPU has no data or instruction cache**: the Cortex-M4 core (Armv7E-M, with FPU,
  DSP extensions and an 8-region Arm MPU, per Table 1.1) has no built-in L1 caches. That
  is a property of the Cortex-M4 itself. So the only cache on the path to memory is
  FCACHE, and it serves **only the code flash**. Accesses to SRAM (stack, locals, globals,
  non-`const` arrays) are not cached but are fast on-chip accesses anyway.
- Related, but not caches: Memory Mirror Function (MMF), which maps a code-flash image to
  a mirror address, and the MPUs (memory protection).

### Clock

- **Maximum CPU clock (ICLK): 240 MHz.** Other maxima from §2: PCLKA 120 MHz,
  BCLK 120 MHz, FCLK 60 MHz.
- Main clock oscillator (MOSC) range: 8–24 MHz, multiplied up by the PLL.
- **The clock the board is actually programmed to** is read at run time:
  `hal_entry.c` stores `SystemCoreClock` in `g_core_clock_hz`. Use that value in the
  report, and you can cross-check it in e2studio → configuration.xml → *Clocks* tab.
  For the SK-S7G2 Blinky project this should be 240 MHz.
  *TODO: confirm from `g_core_clock_hz` = ________ Hz.*

---

## Q2 – Timing the original code

- Code: `phods_v0_original` in `src/phods_baseline.h` (original, unchanged).
- Timing: `DWT->CYCCNT` is enabled with the course's init code (`hal_entry.c`), then read
  right before and after each single call (`phods_runs.h`, `PHODS_RUNNER`).
  Time = cycles / `SystemCoreClock`.
- 10 runs: min / max / avg in `g_results[0]`.

| | min | max | avg |
|---|---|---|---|
| cycles | | | |
| µs | | | |

## Q3 – Code transformations

Each step is its own function. All of them give the same motion vectors as the original
(`test/host_test.c`, plus `g_all_correct` on the board).

| step | transformation | why |
|---|---|---|
| v1 `phods_v1_hoist` | loop-invariant code motion: `B*x`, `B*y` once per block; `vectors_x/y[x][y]` kept in locals; row bound check moved out of the `l` loop; init loop removed | the original recomputes the same address arithmetic and re-reads the output arrays for every pixel |
| v2 `phods_v2_bounds` | bounds-check elimination: compute the valid row/column range of each candidate once, then the inner loop has no `if`; whole out-of-frame rows use a precomputed Σ\|cur\| | 4 comparisons + a branch per pixel removed |
| v3 `phods_v3_share` | data reuse / CSE: the `i = 0` candidate is the same block in the X and Y searches, so it is computed once | 6 → 5 SADs per step (about −17 % work) |
| v4 `phods_v4_copy` | copy the current block (`const`, so in Flash) into a local RAM buffer once per block | tests the Hint: does moving the Flash data to RAM help? |

Expected effect of v4, to be confirmed with the measurements: probably small or none.
The image data is `const`, so it is in code flash behind FCACHE, and after the first
access most reads hit the cache. The local buffer, on the other hand, adds a copy per
block, and at `-O0` every local is itself a stack (SRAM) access. The same reasoning
explains why "keep values in local variables" style optimizations help less than
expected at `-O0`: the locals still live in memory, not in registers.

| step | min µs | max µs | avg µs | speed-up vs v0 |
|---|---|---|---|---|
| v0 | | | | 1.00× |
| v1 | | | | |
| v2 | | | | |
| v3 | | | | |
| v4 | | | | |

## Q4 – DSE over square block size B

- B ∈ {1, 2, 5, 10} (common divisors of N = M = 10), using the Q3 code
  (`phods_dse_<b>_<b>`). If v4 was faster than v3, set `PHODS_DSE_COPY_BLOCK 1` in
  `src/phods_instances.h` and re-run.
- Best B: ____. Relation to Q1: discuss in terms of how many blocks are processed
  (fixed overhead per block), how large a block row is compared to FCACHE / flash
  access width, and how often a candidate falls outside the frame.

## Q5 – DSE over rectangular Bx × By

- All 16 pairs Bx | N, By | M (`phods_dse_<bx>_<by>`).
- Best (Bx, By): ____.
- Heuristic to shrink the search (suggestion): the work per pixel is roughly the same
  for every configuration, and the extra cost grows with the number of blocks
  (N/Bx)·(M/By). So start from the largest blocks and search only around the best
  square B from Q4. Prefer a larger By (contiguous pixels within a row, which is better
  for the flash/cache line) over a larger Bx.

## Plot

`tools/analyze.py` produces `q2_q3.png`, `q4.png`, `q5.png`, and `q2_q5_all.png`, the
single line plot that the exercise asks for.

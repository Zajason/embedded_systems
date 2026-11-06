# Lab 1: full explanation of the code (Part 1 and Part 2)

For each question: what it asks, what the code does, and a line-by-line walk through
the important parts. Measured numbers and the written answers are in `ANSWERS.md`.
How to run Part 1 on the lab PC is in `LAB_GUIDE.md`.

- [Part 1: PHODS on the Renesas S7G2](#part-1-phods-on-the-renesas-s7g2)
  - [Background: the PHODS algorithm](#background-the-phods-algorithm)
  - [Q1 – board characteristics](#q1--board-characteristics)
  - [Q2 – timing the original](#q2--timing-the-original)
  - [Q3 – code transformations](#q3--code-transformations)
  - [Q4/Q5 – block-size design space exploration](#q4q5--block-size-design-space-exploration)
  - [Supporting code (host test, analyze.py)](#supporting-code)
- [Part 2: gem5 design space exploration](#part-2-gem5-design-space-exploration)

---

# Part 1: PHODS on the Renesas S7G2

## File map

| file | role |
|---|---|
| `src/hal_entry.c` | entry point: enables the cycle counter, runs every configuration 10×, stores the statistics |
| `src/phods_config.h` | `N`, `M`, `B`, number of runs |
| `src/phods_baseline.h` | Q2: the original algorithm |
| `src/phods_variants.h` | Q3 step v1 |
| `src/phods_impl.h` | "template" for the optimized code: v2, v3, v4 and all DSE configurations |
| `src/phods_instances.h` | includes the template once per configuration |
| `src/phods_runs.h` | timing wrapper + the table of all 21 configurations |
| `test/host_test.c` | correctness test on a normal PC |
| `tools/analyze.py` | GDB dump → CSV + plots |

## Background: the PHODS algorithm

`phods_v0_original` in `src/phods_baseline.h` is the reference. Every other version
must produce exactly the same motion vectors.

- **Blocks.** The current frame (`N×M = 10×10`) is split into `B×B` blocks. With B = 5
  there are 2×2 = 4 blocks.
- **Goal.** For each block, find the offset `(vectors_x, vectors_y)` where it best
  matches the **previous** frame, i.e. how much the block moved.
- **Search.** 3 steps with step size `S = 4, 2, 1`. In each step:
  - **X search:** try `i = -S, 0, +S` rows of extra offset and keep the best as `bestx`.
  - **Y search:** separately try `i = -S, 0, +S` columns and keep the best as `besty`.
  - Add `bestx`, `besty` to the vector. The maximum total offset is 4+2+1 = ±7
    (that's what the unused `#define p 7` meant).
- **Cost function: SAD** (sum of absolute differences) over the block's pixels:
  ```c
  if(p1-p2>0) distx += p1-p2; else distx += p2-p1;
  ```
- **Outside the frame.** If a pixel of the shifted block lies outside the frame it
  counts as `p2 = 0`. That's the 4-comparison `if` that runs **for every pixel**.
- **Ties.** Comparisons use strict `<`, so on a tie the earlier candidate wins
  (`-S`, then `0`, then `+S`). All optimized versions keep this order, which is why
  their results are bit-identical.

Work per frame: (number of blocks) × 3 steps × 6 SADs × B² pixels.

---

## Q1 – board characteristics

The memory sizes and caches come from the datasheet (see `ANSWERS.md`). The clock
frequency is read from code:

```c
volatile uint32_t g_core_clock_hz;      // hal_entry.c
...
g_core_clock_hz = SystemCoreClock;      // hal_entry.c:67
```

- `SystemCoreClock` is the standard CMSIS global with the CPU frequency in Hz. The
  Renesas startup code sets it after configuring the PLL.
- It is copied into a global so you can watch it in the debugger. The timing code also
  uses it to turn cycles into µs.
- `volatile` forces the compiler to really store the value, even though the program
  never reads it back; the debugger does.

---

## Q2 – timing the original

### 1. Enabling the cycle counter (`hal_entry.c:62`, code given by the course)
```c
CoreDebug->DEMCR |= 0x01000000;   // bit 24 TRCENA: power up the debug/trace units (DWT, ITM)
ITM->LAR = 0xC5ACCE55;            // unlock key for the debug registers
DWT->CYCCNT = 0;                  // reset the cycle counter
DWT->CTRL |= 1;                   // bit 0 CYCCNTENA: start counting
```
- **DWT** (Data Watchpoint and Trace) is a debug unit inside the Cortex-M4.
- **`CYCCNT`** is a 32-bit counter that increases by 1 every CPU clock cycle. At
  240 MHz it wraps after about 17.9 s, much longer than one PHODS call.

### 2. Timing exactly one call (`phods_runs.h:28`)
```c
#define PHODS_RUNNER(fn, BX, BY)                       \
  static int fn##_vx[N/(BX)][M/(BY)];                  \
  static int fn##_vy[N/(BX)][M/(BY)];                  \
  static uint32_t fn##_run(void)                       \
  {                                                    \
    uint32_t t0 = PHODS_CYCLES();                      \
    fn(PHODS_CUR, PHODS_PREV, fn##_vx, fn##_vy);       \
    return (uint32_t)(PHODS_CYCLES() - t0);            \
  }
```
- **What it generates.** For a version `fn`: two `static` output arrays and a function
  `fn_run()` that reads the counter, calls `fn` once, reads it again and returns the
  difference. `##` glues tokens, so `phods_v0_original##_run` → `phods_v0_original_run`.
- **Only the call is measured.** The loop, the statistics and the result storage are
  outside the two counter reads.
- **Wrap-around is harmless.** Unsigned subtraction gives the right difference even if
  the counter wrapped in between.
- **Portable.** `PHODS_CYCLES()` is `DWT->CYCCNT` in `hal_entry.c` and `0` in the PC
  test, so the same header compiles on both.
- **Static arrays** keep the vectors in RAM instead of the small stack, and keep them
  around afterwards for the correctness check.

### 3. Shaping the images
```c
#define PHODS_CUR  ((const int (*)[M])current)
```
- **The mismatch.** `images.h` defines the frames as flat 1-D arrays
  (`const int current[] = {…}`), but the function expects `[N][M]`.
- **The fix.** The cast reads the same memory as rows of `M` ints. Only the first
  N·M = 100 values are used, exactly what the original skeleton did implicitly.

### 4. The 10 runs and the statistics (`hal_entry.c:72`)
```c
for(r = 0; r < PHODS_RUNS; r++) {
  cyc = cfg->run();            // one timed call
  res->cycles[r] = cyc;        // keep every run
  sum += cyc;
  if(cyc < mn) mn = cyc;
  if(cyc > mx) mx = cyc;
}
res->avg_cycles = (float)sum / PHODS_RUNS;
res->min_us     = (float)mn / mhz;     // mhz = SystemCoreClock / 1e6 → cycles / (cycles per µs)
```
- **Result storage.** Everything goes into `g_results[c]` (a `volatile` array of
  structs). Each entry holds: name, question, Bx/By, the 10 raw cycle counts,
  min/max/avg in cycles and µs, and a correctness flag.
- **`float` is cheap here:** the Cortex-M4F has a single-precision FPU.

### 5. Stopping for you
```c
g_done = 1;
__BKPT(0);
```
- **Under the debugger,** the breakpoint instruction halts the CPU right here, with all
  results ready. You don't need to place a breakpoint yourself.
- **Without a debugger** this would fault, but in this lab it always runs under one.

---

## Q3 – code transformations

There is one function per step, so the effect of each step can be measured on its own:

| step | function | file |
|---|---|---|
| v0 | `phods_v0_original` | `phods_baseline.h` |
| v1 | `phods_v1_hoist` | `phods_variants.h` |
| v2 | `phods_v2_bounds` | template `phods_impl.h`, sharing off, copy off |
| v3 | `phods_v3_share` | template, sharing on, copy off |
| v4 | `phods_v4_copy` | template, sharing on, copy on |

### v1 – loop-invariant code motion / scalar replacement (`phods_variants.h`)

1. **Block position once per block**, not per pixel:
   ```c
   bx = B*x;   by = B*y;
   ```
   The original computes `B*x + vectors_x[x][y] + i + k` several times **per pixel**.
2. **Motion vector in locals**, not in the output array:
   ```c
   vx = 0; vy = 0;
   ...
   vx += bestx; vy += besty;          // end of each step
   ...
   vectors_x[x][y] = vx;              // written once, at the end
   ```
   - **Saving:** the original reads `vectors_x[x][y]` about 4 times per pixel, each time
     with a 2-D address calculation and a memory read.
   - **Side effect:** since the vectors start at 0 here, the separate zeroing loop is
     gone too.
3. **Row check moved out of the pixel loop.** The row depends only on `k`:
   ```c
   cp  = &current[bx+k][by];          // pointer to this row of the block
   row = bx + vx + i + k;
   if(row < 0 || row > N-1) {         // whole row outside the frame → p2 = 0
     for(l=0; l<B; l++) { d = cp[l]; dist += (d < 0) ? -d : d; }
   } else {
     for(l=0; l<B; l++) {
       col = by + vy + l;
       d = (col < 0 || col > M-1) ? cp[l] : cp[l] - previous[row][col];
       dist += (d < 0) ? -d : d;
     }
   }
   ```
   - `cp[l]` replaces `current[B*x+k][B*y+l]`, so there's no 2-D index per pixel.
   - `(d<0)?-d:d` is the same absolute value as the original if/else.
   - `d = cp[l]` is the off-frame case `p1 - 0`.
4. **`bestx = besty = 0`** at the start of each step. The original left them
   uninitialised; the result is the same.

### The template mechanism (`phods_impl.h` + `phods_instances.h`)

C has no templates. Instead, `phods_impl.h` has **no include guard** and is included
several times with different parameters:
```c
#define PHODS_FN phods_v3_share     // name of the function to generate
#define PHODS_BX B                  // block rows
#define PHODS_BY B                  // block columns
#define PHODS_SHARE_CENTER 1        // switch for optimization 3
#define PHODS_COPY_BLOCK 0          // switch for optimization 4
#include "phods_impl.h"             // → generates phods_v3_share() and phods_v3_share_sad()
```
- **Fixed block size per copy.** Every copy is a separate function with its block size
  as a compile-time constant, like `B` in the original, so the comparison is fair.
- **Clean-up.** At the end of the file, `#undef` clears the parameters for the next
  include.

### v2 – bounds-check elimination: the SAD helper (`phods_impl.h:43`)

```c
static int PHODS_SAD(const int *cb, int cs, const int *rowabs,
                     const int prev[N][M], int r, int c)
```
Its inputs:
- `cb`, `cs`: the current block and its row stride (`M` for the image in Flash, `BY`
  for the RAM copy in v4).
- `rowabs[k]`: the precomputed Σ|pixel| of each block row.
- `r, c`: top-left corner of the candidate in the previous frame (may be negative or
  past the edge).

**Step 1 – which part of the candidate is inside the frame** (lines 52–65):
```c
kmin = (r < 0) ? -r : 0;          // first block row inside the frame
kmax = N - r;  (clamped to BX)    // one past the last block row inside
lmin = (c < 0) ? -c : 0;          // same for columns
lmax = M - c;  (clamped to BY)
if(lmin == lmax) kmax = kmin;     // no column inside → treat every row as outside
```
Example: block row r0 = 5, vx = 0, X candidate `+S = 4` gives r = 9 and
`kmax = 10 − 9 = 1`. Only block row 0 is inside; rows 1–4 are below the frame.

**Step 2 – whole rows outside the frame cost one addition each:**
```c
for(k=0; k<kmin; k++)        sum += rowabs[k];
for(k=kmax; k<PHODS_BX; k++) sum += rowabs[k];
```
- **Why this is right:** outside the frame `p2 = 0`, so each pixel adds `|p1|`, and the
  row's total is already in `rowabs[k]`.
- **When `rowabs` is filled:** once per block (line 115), because the current block
  doesn't change during its search.

**Step 3 – rows inside the frame:**
```c
cp = cb + k*cs;                         // current block row
pp = &prev[r+k][c+lmin];                // first in-frame pixel of the candidate row
for(l=0;    l<lmin;     l++) sum += |cp[l]|;          // left of the frame
for(l=lmin; l<lmax;     l++) sum += |cp[l] - *pp++|;  // inside: NO checks at all
for(l=lmax; l<PHODS_BY; l++) sum += |cp[l]|;          // right of the frame
```
- **The middle loop is the hot loop.** It is now just subtract, absolute value, add,
  with a pointer walking along the row.
- **The `lmin == lmax` line** was added after the PC test's undefined-behaviour checker
  caught a pointer formed before the start of a row in that corner case.

### v3 – data reuse: share the centre candidate (`phods_impl.h:135`)
```c
#if PHODS_SHARE_CENTER
  dc = SAD(r0+vx, c0+vy);               // the i = 0 candidate, computed ONCE
#endif
  d = SAD(r0+vx-S, c0+vy);  if(d < min1) { min1 = d; bestx = -S; }
  d = dc;                   if(d < min1) { min1 = d; bestx = 0;  }   // reused
  d = SAD(r0+vx+S, c0+vy);  if(d < min1) { min1 = d; bestx = S;  }

  d = SAD(r0+vx, c0+vy-S);  if(d < min2) { min2 = d; besty = -S; }
  d = dc;                   if(d < min2) { min2 = d; besty = 0;  }   // reused
  d = SAD(r0+vx, c0+vy+S);  if(d < min2) { min2 = d; besty = S;  }
```
- **Why it's valid:** the X search at `i = 0` and the Y search at `i = 0` compare the
  **same** block at offset `(vx, vy)`, because the vector is only updated after both
  searches.
- **Saving:** 6 → 5 SADs per step, about 17 % less pixel work.
- **Same results:** order and strict `<` are kept, so ties go the same way.
- **v2** is this same code with sharing turned off, so optimization 2 can be measured
  alone.

### v4 – copy the current block from Flash to RAM (`phods_impl.h:104`)
```c
#if PHODS_COPY_BLOCK
  for(k...) for(l...) blk[k*PHODS_BY + l] = cur[r0+k][c0+l];   // const (Flash) → local (stack/SRAM)
  cb = blk; cs = PHODS_BY;
#endif
```
- **The idea:** the current block is read 3 steps × 5 SADs = 15 times, so copy it once.
  Nothing else changes, because the SAD only sees `cb`/`cs`.
- **This tests the exercise's hint:** only `const` data is in Flash.
- **Expected result:** little or no gain. FCACHE already serves repeated Flash reads,
  the copy costs time, and at `-O0` the local array is also memory, not registers.

---

## Q4/Q5 – block-size design space exploration

### The 16 functions (`phods_instances.h`)
Each `(Bx, By)` with Bx, By ∈ {1, 2, 5, 10} is a separate copy of the template:
```c
#define PHODS_FN phods_dse_2_5
#define PHODS_BX 2
#define PHODS_BY 5
#define PHODS_SHARE_CENTER PHODS_DSE_SHARE_CENTER   // 1
#define PHODS_COPY_BLOCK PHODS_DSE_COPY_BLOCK       // 0 (line 23); set 1 if v4 won on the board
#include "phods_impl.h"
```
- **Which code:** the Q3 code, as the exercise asks ("the code from question 3").
- **What changes with block size:** with Bx = 2, By = 5 there are (10/2)×(10/5) = 10
  blocks, the output arrays are `[5][2]` and the start minimum is `255*2*5`. All of
  these are fixed at compile time.

### The configuration table (`phods_runs.h`)
The block sizes are listed once, as an "X-macro":
```c
#define PHODS_DSE_LIST(X) \
  X(1,1)  X(1,2)  X(1,5)  X(1,10)  \
  X(2,1)  ...               X(10,10)
```
The list is then expanded twice:
```c
#define PHODS_DSE_RUNNER(bx, by) PHODS_RUNNER(phods_dse_##bx##_##by, bx, by)
PHODS_DSE_LIST(PHODS_DSE_RUNNER)                 // 16 timing wrappers

#define PHODS_DSE_ENTRY(bx, by) PHODS_ENTRY(5, phods_dse_##bx##_##by, bx, by)
static const phods_config_t phods_configs[] = {
  PHODS_ENTRY(2, phods_v0_original, B, B)        // Q2
  PHODS_ENTRY(3, phods_v1_hoist,    B, B)        // Q3 …
  ...
  PHODS_DSE_LIST(PHODS_DSE_ENTRY)                // 16 DSE rows
};
```
- **Result:** `phods_configs[]` has 21 rows. Each holds a name (`#fn` turns it into a
  string), the question, Bx/By, the function to time and pointers to its vector arrays.
- **Adding a configuration** = adding one line; `hal_entry.c` just loops over the table.
- **Q4 vs Q5:** Q4 (square B) is the Bx = By subset of Q5, so it's measured once.
  `analyze.py` splits them for the plots.

### Correctness on the board (`hal_entry.c:43`, `:99`)
```c
if(cfg->bx == B && cfg->by == B) {
  res->correct = same_vectors(cfg, &phods_configs[0]);   // compare with the original
  if(!res->correct) g_all_correct = 0;
} else res->correct = -1;   // the original only exists for B = 5
```
`same_vectors` compares the vector arrays as flat lists, through the pointers stored in
the table.

---

## Supporting code

### `test/host_test.c` – correctness on a PC
- **`reference()`** is the original algorithm line for line, with `B` replaced by
  run-time `bx`, `by`, so it can check **every** block size.
- **Check 1:** all 21 configurations against the reference on the real images.
- **Check 2:** on the real images most vectors come out 0, so check 1 alone is weak.
  The test therefore also uses 2000 random frame pairs (values −128…127, half of them
  shifted copies so real motion exists), compared for every DSE function and v1/v2/v4.
  v3 is the same code as `dse_5_5`.
- **Sanitizer:** it compiles with `-fsanitize=undefined`.
- **Run it** with `make -C test run`. Everything passes.

### `tools/analyze.py` – GDB dump → CSV + plots
- **Input format.** GDB prints `{name = 0x… "phods_v0_original", question = 2, bx = 5, …}, …`.
- **Parsing.**
  - Split at each `{name =` and read the quoted name.
  - Remove the `cycles = {…}` array first, so its numbers can't be mistaken for other
    fields (GDB may print `40000 <repeats 9 times>`).
  - Read each field with a regex.
- **Output.** `results.csv`, a printed table, and `q2_q3.png`, `q4.png`, `q5.png`,
  `q2_q5_all.png` (the single Q2–Q5 line plot the exercise asks for), each with
  avg/min/max lines.

---

# Part 2: gem5 design space exploration

The results and their interpretation are in `ANSWERS.md`. This section explains the
code. Everything is in `part2/scripts/`, mounted into the container as
`/workspace/scripts`.

## What the course provides

From `github.com/aferikoglou/Gem5DSE`:
- **`tables_UF/tables.c`:** four `malloc`'d `double` arrays of N = 100,000, an init loop
  and the compute loop `y[i] = y[i] + a1*x1[i] + a2*x2[i] + a3*x3[i]`.
- **`tables_ufXX.c`:** the same program with the compute loop unrolled XX times, plus
  a remainder loop for N % XX elements:
  ```c
  for (int i = 0; i <= N - 4; i = i + 4) {        // UF = 4
      y[i]   = y[i]   + a1*x1[i]   + a2*x2[i]   + a3*x3[i];
      y[i+1] = y[i+1] + a1*x1[i+1] + a2*x2[i+1] + a3*x3[i+1];
      ...                                          // i+2, i+3
  }
  for (int i = N - (N % 4); i <= N - 1; i++) ...   // leftovers (none here: 100000 % 4 = 0)
  ```
- **`compile.sh`:** compiles all of them with `gcc -O0`.
- **`genOptimizer.py` + `pmodules/optimizationProblem.py`:** NSGA-II via `pymoo` (Q5).
- **gem5's own `configs/learning_gem5/part1/simple.py`** (CPU + DRAM, no caches) and
  **`two_level.py`** (L1I + L1D per CPU, shared L2), with the cache classes in
  `caches.py`: L1 2-way, 2-cycle tag/data/response latency; L2 8-way, 20-cycle; 64 B lines.

## `setup_container.sh` – the course's "Exercise Setup", automated

The container is the course's pre-built gem5 image. It contains a compiled
`gem5.opt` but not the exercise files, so this script does the README's setup steps:
```bash
git clone … Gem5DSE /tmp/Gem5DSE && cp -r /tmp/Gem5DSE/* /gem5/   # exercise files into /gem5
(cd /gem5/tables_UF && bash compile.sh)                            # build tables*.exe (x86, -O0)
```
Then it patches `simple.py`. The README says to "modify simple.py … to take as input
the path to an .exe file"; `two_level.py` already does that, `simple.py` hard-codes the
hello-world binary:
```python
s = s.replace("binary = os.path.join(",
              "import sys\nbinary = sys.argv[1] if len(sys.argv) > 1 else os.path.join(", 1)
```
- **How the argument arrives:** gem5 passes everything after the config script name to
  the script in `sys.argv`.
- **Effect:** `simple.py /gem5/tables_UF/tables.exe` now simulates that program, and
  without an argument it still runs hello-world.

Finally it `pip install`s `pandas`, `paretoset` and `pymoo` (versions pinned by the
course's `requirements.txt`).

## `gem5sim.py` – running one simulation

```python
def run(config, binary, l1i=None, l1d=None, l2=None, tag=None):
    outdir = os.path.join(OUT_ROOT, tag)                 # e.g. /tmp/gem5runs/two_level_tables_uf8_2kB_16kB_128kB
    cmd = [GEM5_BIN, "--outdir=" + outdir, config, binary]
    if l1i: cmd.append("--l1i_size=" + l1i)              # same flags as in the exercise
    ...
    res = subprocess.run(cmd, stdout=log, stderr=subprocess.STDOUT, cwd=GEM5)
    return num_cycles(os.path.join(outdir, "stats.txt")), wall
```
- **Same command as the exercise:** it builds exactly the exercise's
  `build/X86/gem5.opt configs/…/two_level.py /gem5/tables_UF/tables_ufXX.exe --l1i_size=…`.
- **`--outdir`.** By default gem5 writes to `./m5out`, so two simulations at the same
  time would overwrite each other's `stats.txt`. Giving every configuration its own
  folder is what makes the parallel runs in Q3/Q4 safe. The folder also keeps
  `gem5.log` and the full `stats.txt` for later inspection; that's where the cache miss
  counts in `ANSWERS.md` come from.
- **Reading the result:**
  ```python
  if line.startswith("system.cpu.numCycles"):
      return int(float(line.split()[1]))
  ```
  This is the same statistic the exercise names. Splitting on whitespace is more robust
  than the course script's `line.split(" ")[25]`, which depends on the exact number of
  spaces.
- **Errors:** a non-zero gem5 exit code raises an error that points to the log, so a
  failed simulation can never silently become a data point.
- **Lists of values:** `L1_SIZES`, `L2_SIZES` and `UFS` are the value sets from the
  exercise. `exe(uf)` returns `tables.exe` for `uf=None` and `tables_uf<uf>.exe`
  otherwise.

## `q1_q2.py`

Two calls:
```python
g.run(g.SIMPLE, g.exe())                                   # Q1: no caches
g.run(g.TWO_LEVEL, g.exe(), "8kB", "8kB", "128kB")         # Q2: the given cache sizes
```
It prints the cycles and writes `results/q1_q2.csv`.

## `q3.py`

```python
with ProcessPoolExecutor(WORKERS) as ex:
    ufs = list(ex.map(uf_job, [None] + g.UFS))   # tables.exe (shown as UF=1) + uf2..uf32, all 8/8/128
    l1d = list(ex.map(l1d_job, g.L1_SIZES))      # tables.exe with L1D 2..64 kB, L1I 8 kB, L2 128 kB
```
- **Parallelism:** `ProcessPoolExecutor` runs `WORKERS` simulations at once. Each job is
  an independent gem5 process.
- **Order:** `ex.map` returns results in input order, so the CSVs come out sorted.
- **Baseline:** `tables.exe` is included as "UF = 1" so the plot shows the effect of
  unrolling relative to no unrolling.

## `q4.py` – exhaustive search + Pareto front

```python
space = list(itertools.product(g.L1_SIZES, g.L1_SIZES, g.L2_SIZES, g.UFS))   # 6*6*4*5 = 720
```
- **Resumable.** Rows already in `q4_exhaustive.csv` are read back into `done` and
  skipped. Each new result is appended and `flush()`ed immediately (from
  `as_completed`, in whatever order they finish), so stopping the script loses nothing.
- **Per row:** the 4 parameters, `totalMemoryKB = L1I + L1D + L2` (the second objective,
  as the exercise defines it), `latencyCC = numCycles`, and the simulation's own wall
  time.
- **Timing:** total wall time goes to `q4_runtime.txt`. The **sum of the per-simulation
  times** is printed as well, because that's what a sequential exhaustive search costs.
  That's the fair number to compare against the GA, which runs one simulation at a time.

Pareto front, exactly as the exercise describes ("a pandas dataframe with two columns
totalMemoryKB and latencyCC"):
```python
data = df[["totalMemoryKB", "latencyCC"]]
mask = paretoset(data, sense=["min", "min"], distinct=False)
po = df[mask].sort_values(["totalMemoryKB", "latencyCC"])
po.to_csv("q4_pareto.csv")          # Pareto points WITH their L1I/L1D/L2/UF
```
- **What `paretoset` returns:** a boolean mask that is True for every row that no other
  row beats in both objectives (≤ in both and < in one).
- **`distinct=False`:** keeps ties, so two configurations with identical
  (memory, latency) would both be reported.
- **Why the mask:** applying it to the full dataframe keeps the architecture columns
  next to each Pareto point, as the exercise asks.

## `q5.py` – the genetic algorithm

It runs the course's `genOptimizer.py` unchanged, from `/gem5`: the course script uses
relative paths, `./m5out/stats.txt` and `from pmodules…`. `q5.py` times the run and
saves the log.

How `genOptimizer.py` works:
- **Encoding.** Each variable is an integer index: `L1 = 2·2^x` kB (x = 0..5),
  `L2 = 128·2^x` kB (x = 0..3), `UF = 2·2^x` (x = 0..4).
- **Objectives.** `OptimizationProblem._evaluate` runs one gem5 simulation and returns
  `[numCycles, totalMemoryKB]`, both minimised.
- **Algorithm: NSGA-II.**
  - Start from 10 random integer configurations.
  - Each generation, create 5 offspring with SBX crossover and polynomial mutation,
    rounded back to integers (`RoundingRepair`).
  - Keep the best 10 by non-dominated sorting + crowding distance.
  - `eliminate_duplicates` avoids re-simulating an identical configuration within a
    generation.
- **Stopping.** `n_max_gen = 5`, which is what actually stops it here
  (10 + 4×5 = 30 evaluations).
- **Output.** `genOptimizer_PO.csv` with the final non-dominated set; `q5.py` copies it
  to `results/`.

**Counting the evaluations.** pymoo's progress table has rows like
`5 |  30 | 6 | …` (`n_gen | n_eval | …`). `q5.py` finds them with a regex and takes the
last `n_eval`, giving 30. gem5's own output is mixed into the same log, which is why the
regex only matches lines that start with two `|`-separated integers.

## `plot_part2.py`

It reads the CSVs and draws:
- `q3_unrolling.png`: cycles vs UF, log₂ x-axis, each point labelled in millions.
- `q3_l1d.png`: cycles vs L1D size.
- `q4_q5_pareto.png`:
  - all 720 configurations as a scatter;
  - the exhaustive Pareto front in red;
  - the NSGA-II front in green.

  It uses a log₂ x-axis for total memory, so the 132–1152 kB range is readable.

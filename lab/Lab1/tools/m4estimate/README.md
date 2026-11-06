# Cortex-M4 cycle estimates (no board needed)

Rough **estimates** of the board time of our v1–v4 and DSE variants, made before the lab so
there is something to compare against. These are not measurements.

How it works:
1. `clang --target=thumbv7em-none-eabihf -mcpu=cortex-m4 -O0` compiles our code and a past
   student's code (jimmichalako/embedded-systems-ntua, `lab-1/src/ask1/{base,rect}_phods.c`,
   all 16 Bx×By variants) to ARM object files.
2. `objload.py` relocates them, and `sim.py` runs each PHODS call in the Unicorn emulator,
   counting instructions, 32-bit instructions, loads, stores and taken branches
   (→ `features.json`).
3. `fit2.py` fits simple linear cycle models on the past student's 17 *measured* board
   times and keeps the three with the lowest leave-one-out error (mean 3–6 %, worst
   15–20 %). It then applies them to our code (→ `pred.json`, min/median/max of the
   three models).

Check: our v0 executes the same instructions as their original (193,811 vs 193,799), so
their measured 1821 µs carries over to our v0 directly.

Limits: the board uses GCC, not clang, so `-O0` code differs. Flash wait states and
FCACHE are not modelled, only fitted on average. Treat the numbers as ±15 %.

Re-run: `./run.sh` (downloads their two .c files, rebuilds, emulates, fits).


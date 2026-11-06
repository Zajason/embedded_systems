# Part 2 (Ζητούμενο 2): gem5 – how to run

Everything runs on your own computer through Docker, using the course's pre-built image
`aferikoglou/gem5built:v1.0` (gem5 already compiled, x86).

```
part2/
  scripts/
    setup_container.sh   course's "Exercise Setup": Gem5DSE files, compile tables*.exe,
                         make simple.py take the binary, pip install pandas/paretoset/pymoo
    gem5sim.py           helper: run one simulation in its own folder, read numCycles
    common_io.py         where CSVs go (part2/results)
    q1_q2.py             Q1 simple.py, Q2 two_level.py (8kB/8kB/128kB)
    q3.py                Q3 unrolling-factor sweep + L1D-size sweep
    q4.py                Q4 exhaustive 720-point DSE + Pareto front (paretoset)
    q5.py                Q5 runs the course's genOptimizer.py (NSGA-II), times it
    plot_part2.py        all plots from the CSVs (run on the Mac)
  results/               CSVs, logs and PNGs (already filled in)
```

## Run it again from scratch (Mac, Apple Silicon)

1. Docker Desktop → Settings → General → enable **"Use Rosetta for x86_64/amd64
   emulation on Apple Silicon"** (much faster than QEMU).
2. Pull the image once (~7 GB):
   ```bash
   docker pull aferikoglou/gem5built:v1.0
   ```
3. Start a long-lived container with `part2/` mounted as `/workspace`:
   ```bash
   cd ~/university/embedded_systems/lab/Lab1/part2
   docker run -d --name gem5lab --platform linux/amd64 -v "$PWD":/workspace aferikoglou/gem5built:v1.0 sleep infinity
   ```
4. One-time setup inside it. This takes ~10 min: the pip install is slow under emulation.
   ```bash
   docker exec gem5lab bash /workspace/scripts/setup_container.sh
   ```
5. Run the questions (`WORKERS` = parallel simulations):
   ```bash
   docker exec -w /workspace/scripts gem5lab python3 q1_q2.py
   ```
   ```bash
   docker exec -w /workspace/scripts -e WORKERS=8 gem5lab python3 q3.py
   ```
   ```bash
   docker exec -w /workspace/scripts -e WORKERS=8 gem5lab python3 q4.py
   ```
   ```bash
   docker exec -w /workspace/scripts gem5lab python3 q5.py
   ```
   `q4.py` can be stopped and restarted: finished configurations are kept in
   `results/q4_exhaustive.csv`.
6. Plots (on the Mac):
   ```bash
   python3 -m pip install pandas matplotlib
   ```
   ```bash
   python3 scripts/plot_part2.py
   ```
7. Clean up:
   ```bash
   docker rm -f gem5lab
   ```

On Linux/x86 Windows drop `--platform linux/amd64`. If you prefer the interactive way
from the exercise (`docker run --user root --volume ./workspace:/workspace --rm -it …`),
run `bash /workspace/scripts/setup_container.sh` first in that shell. The exercise's
commands then work exactly as written, e.g.
`build/X86/gem5.opt configs/learning_gem5/part1/simple.py /gem5/tables_UF/tables.exe`.

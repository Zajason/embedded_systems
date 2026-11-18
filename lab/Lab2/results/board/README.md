# Measurements on the AUP-ZU3

Made with `Source_codes/yolo_nms/board_measure.sh` (see `../../BOARD_GUIDE.md`) on
2026-11-18. The board's clock was wrong, so dates inside the files say 2094. Environment: `environment.txt`.

- `qN/summary.csv`: one line per binary.
  - `rep1_ms`..`rep3_ms`: mean of the 20 `ms=` lines of each of the 3 executions
  - `mean_ms`, `var_ms2`: mean and sample variance of those 3
  - `l2d_refill_runs1` / `l2d_refill_runs0`: `perf stat -r 20 -e l2d_cache_refill` with one pass
    over the frames / only loading the data; nms() itself is the difference
- `qN/raw/`: every program and perf output as printed.
- `measure.log`: console log of the Q2-Q4 run (built with `--skip-validate`; all versions had
  passed `-DVALIDATE` on the PC).

Memory footprints are in `../q1`..`../q4` (Massif and `-DALLOC_STATS`, measured on the PC).

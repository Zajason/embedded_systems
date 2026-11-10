# Ζητούμενο 1: original NMS (`bin/nms-orig`), memory footprint

Measured with Massif in an Ubuntu 22.04 arm64 container (same architecture and glibc as the
board, so the heap numbers carry over). Build: `./build.sh orig` (g++ -O3).

```
valgrind --tool=massif --threshold=0 --peak-inaccuracy=0 \
  --massif-out-file=results/q1/massif_orig.out ./bin/nms-orig input=data/detrac_y5s_640.bin
ms_print --threshold=0 results/q1/massif_orig.out > results/q1/ms_print_orig.txt
python3 tools/massif_nms.py results/q1/massif_orig.out
```

## Peak snapshot (snapshot 22)

| allocation site | bytes | part of nms()? |
|---|---:|---|
| `main`: `res.reserve(4096)` (4096 × 152 B) | 622,592 | no |
| `nms_orig` → `std::vector<Detection>::_M_realloc_insert` (per-class vectors) | 295,488 | **yes** |
| libstdc++ start-up (emergency exception pool) | 72,704 | no |
| `load()`: vector of frame pointers | 2,048 | no |
| `nms_orig` → `std::map` nodes | 448 | **yes** |
| **total useful heap** | **993,280** | |

**nms() footprint at peak = 295,936 B ≈ 289 KiB** (295,488 + 448).
295,488 B = 1,944 Detections of 152 B: the vectors of the busiest frame, sized by the
vector's doubling growth (capacity, not size), which is the main thing the other DDTs
can save on.

The frames themselves are loaded with `mmap` in `main.cpp`, so they are not in the heap.

## Execution time

To be measured **on the board** (`./bin/nms-orig input=data/detrac_y5s_640.bin runs=20`,
average of the 20 `ms=` lines). For reference only, the same binary on the Mac (M-series,
in Docker) prints ≈ 21 ms per run after warm-up, so expect the board's Cortex-A53 to be
several times slower.

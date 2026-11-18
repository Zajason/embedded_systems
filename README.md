# Embedded Systems Design: lab exercises

NTUA ECE, 9th semester (Σχεδίαση Ενσωματωμένων Συστημάτων), 2026.
Achilleas-Odysseas Metsis, Iason Zakynthinos.

| Lab | Topic | Platform | Report |
|---|---|---|---|
| [Lab 1](lab/Lab1) | Loop optimizations and design space exploration of the PHODS motion estimation kernel; cache DSE in gem5 | Renesas S7G2 (Cortex-M4), gem5 | [lab1_report.pdf](lab/Lab1/lab1_report.pdf) |
| [Lab 2](lab/Lab2) | Dynamic data type refinement (DDTR) and dynamic memory management (DMM) of YOLOv5 Non-Maximum Suppression | AUP-ZU3 (Zynq UltraScale+, Cortex-A53) | [lab2_report.tex](lab/Lab2/report/lab2_report.tex) ([preview](lab/Lab2/report/lab2_report_preview.pdf)) |

---

## Lab 1: PHODS on the Renesas S7G2 and gem5

```
lab/Lab1/
├── Exercise1.pdf, Renesas_Starter_Guide.pdf   assignment and board guide
├── lab1_report.pdf                            final report
├── reference/exercise1a_original.c            the original PHODS code of the assignment
├── src/                                       Part 1, board code (e2studio "Blinky" project)
│   ├── hal_entry.c                            measurement loop: DWT->CYCCNT, results in g_results
│   ├── phods_baseline.h, phods_variants.h     original and optimized versions (v1-v4)
│   └── phods_config.h, phods_runs.h, ...      block sizes, run table, test images
├── test/host_test.c, Makefile                 correctness check on a PC: `make -C lab/Lab1/test run`
├── tools/analyze.py                           GDB dump of g_results -> CSV and plots
└── part2/                                     Part 2, gem5
    ├── scripts/                               one script per question, run in the gem5 container
    └── results/                               CSVs, plots and logs of every gem5 run
```

## Lab 2: DDTR and DMM of YOLOv5 NMS on the AUP-ZU3

```
lab/Lab2/
├── Lab2_askisi.pdf, Lab2_AUP-ZU3-Guide.pdf    assignment and board guide
├── BOARD_GUIDE.md                             step by step: code to the board, measuring, results back
├── report/                                    report source for Overleaf (pdfLaTeX) and a preview PDF
├── Source_codes/                              the course repository, with our changes
│   ├── yolo_nms/
│   │   ├── postprocess.cpp                    nms() on DDTR: erase loop (Q2), #ifdef NMS_FLAG loop (Q4)
│   │   ├── allocators/                        arena, first-fit, best-fit (ours), worst-fit (given)
│   │   ├── board_measure.sh                   builds and measures all 91 versions on the board
│   │   ├── test/alloc_stress.cpp              random malloc/free check of the allocators
│   │   └── build.sh, main.cpp, upstream/, data/   given by the course
│   ├── ddtr_student/                          the DDTR library (given)
│   └── perf_setup/                            perf install scripts for the board (given)
├── results/
│   ├── q1 ... q4/                             memory footprints: Massif (glibc) and -DALLOC_STATS
│   └── board/                                 time and l2d_cache_refill measured on the AUP-ZU3
└── tools/massif_nms.py                        sums the nms() part of a Massif peak snapshot
```

Build and check one version (on the board, or any arm64/x86 Linux with g++):
```bash
cd lab/Lab2/Source_codes/yolo_nms
./build.sh double_linked_list arena -DVALIDATE
./bin/nms-double_linked_list-arena-validate input=data/detrac_y5s_640.bin   # validate: OK
```

Main results (AUP-ZU3, one pass of `nms()` over all 192 frames):

| Version | Time (ms) | Footprint (B) |
|---|---:|---:|
| Original (`std::vector`, glibc) | 347.54 | 295,936 |
| Q2 best time: DLLR-NS, glibc | 86.32 | 155,712 |
| Q2 least memory: SLL-NS, glibc | 103.73 | 148,224 |
| Q3 best time: DLL-NS, arena | 54.87 | 162,624 |
| Q4 best: SLLR-NS, arena, flags | 58.95 | 148,816 |

Course lectures and exams are kept out of the repository (copyrighted material).

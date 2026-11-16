# Ζητούμενο 2: memory footprint of nms() per DDT (glibc)

Massif in the Ubuntu 22.04 arm64 container, binaries from `./build.sh <ddt> glibc`
(not -DVALIDATE), input `data/detrac_y5s_640.bin`, default runs. Summed with
`python3 tools/massif_nms.py results/q2/massif_<ddt>.out`. Raw files: `massif_<ddt>.out`,
`ms_print_<ddt>.txt`. Table: `massif_q2.csv`.

At the peak, the busiest frame holds 924 detections:
- vector: 295,488 B = capacity 1,944 x 152 B (doubling growth, about half unused)
- SLL: 147,840 B = 924 nodes x 160 B (152 B Detection + 8 B next)
- DLL: 155,232 B = 924 nodes x 168 B (+ 8 B prev)
The rest (384-1,536 B) is std::map nodes plus, for the sentinel variants, the sentinel nodes;
roving adds 96 B.

`peak_heap_extra_B` is malloc's own header/alignment overhead, which Massif does not count
as useful heap: one 16 B header per list node shows up here (7.5 KB SLL, 15 KB DLL).

Time and LLC misses: to be measured on the board.

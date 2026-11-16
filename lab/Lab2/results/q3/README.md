# Ζητούμενο 3: allocators

`alloc_stats.csv`: `./build.sh <ddt> <allocator> -DALLOC_STATS`, then
`./bin/nms-<ddt>-<allocator>-stats input=data/detrac_y5s_640.bin` (Ubuntu 22.04 arm64 container;
these counts do not depend on the machine). `peak_malloc_bytes` is the `malloc_bytes=` value printed.

- mmap_bytes = 1 MiB for every combination: the busiest frame fits in one 1 MiB piece.
- first-/best-/worst-fit give the same peak: blocks of one DDT all have the same size, so
  every policy finds a block of the same size; they differ in where (and how fast), not in how much.
- arena has no per-block header, so lists need ~16 B less per node (147,840 vs 162,624 for SLL),
  but vector is worse (505,632): an arena never reuses the old buffers of a growing vector until
  the frame's nms() frees everything.

All 36 DDT x allocator binaries print `validate: OK`; `test/alloc_stress.cpp` (random
malloc/free with pattern checks) passes for all four allocators.

Time, Massif-equivalent footprint and LLC misses for the report: on the board.

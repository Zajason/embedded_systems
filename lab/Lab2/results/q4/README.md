# Ζητούμενο 4: flag instead of erase (-DNMS_FLAG)

All 45 DDT x allocator builds with -DNMS_FLAG -DVALIDATE print `validate: OK`.
- `massif_q4.csv`, `massif_<ddt>.out`, `ms_print_<ddt>.txt`: nms() at peak with glibc (Massif).
- `alloc_stats.csv`: -DNMS_FLAG -DALLOC_STATS for arena / first-fit / best-fit / worst-fit.

The flags vector adds one byte per box (+ a header), so the footprints are a few hundred
bytes above Q2/Q3; dets itself never shrinks now, but its peak was the full frame anyway.
Time and l2d_cache_refill (AUP-ZU3): `../board/q4/summary.csv`.

#include <stdint.h>
#include "images.h"
#define PHODS_CYCLES() 0u
#include "phods_runs.h"
void setup(void){}
#define B_(fn) void bench_##fn(void){ (void)fn##_run(); }
B_(phods_v0_original) B_(phods_v1_hoist) B_(phods_v2_bounds) B_(phods_v3_share) B_(phods_v4_copy)
#define BD(bx,by) B_(phods_dse_##bx##_##by)
PHODS_DSE_LIST(BD)

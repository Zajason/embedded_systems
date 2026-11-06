/*
 * phods_runs.h - table of every configuration that gets timed.
 *
 * Needs, before inclusion:
 *   - the images (current[], previous[]) from images.h
 *   - PHODS_CYCLES(): expression returning the current cycle counter
 *     (DWT->CYCCNT on the board; something else / 0 on the host test)
 *
 * Each runner calls one PHODS variant on the frames and returns the number of
 * cycles spent inside that single call (call overhead of the runner itself is
 * not included). Motion vectors are kept in static arrays so they can be
 * checked afterwards.
 */
#ifndef PHODS_RUNS_H_
#define PHODS_RUNS_H_

#include <stdint.h>
#include "phods_config.h"
#include "phods_baseline.h"
#include "phods_variants.h"
#include "phods_instances.h"

/* images.h stores the frames as flat const arrays; view them as [N][M]. */
#define PHODS_CUR  ((const int (*)[M])current)
#define PHODS_PREV ((const int (*)[M])previous)

#define PHODS_RUNNER(fn, BX, BY)                                              \
  static int fn##_vx[N/(BX)][M/(BY)];                                         \
  static int fn##_vy[N/(BX)][M/(BY)];                                         \
  static uint32_t fn##_run(void)                                              \
  {                                                                           \
    uint32_t t0 = PHODS_CYCLES();                                             \
    fn(PHODS_CUR, PHODS_PREV, fn##_vx, fn##_vy);                              \
    return (uint32_t)(PHODS_CYCLES() - t0);                                   \
  }

PHODS_RUNNER(phods_v0_original, B, B)
PHODS_RUNNER(phods_v1_hoist,    B, B)
PHODS_RUNNER(phods_v2_bounds,   B, B)
PHODS_RUNNER(phods_v3_share,    B, B)
PHODS_RUNNER(phods_v4_copy,     B, B)

#define PHODS_DSE_LIST(X) \
  X(1,1)  X(1,2)  X(1,5)  X(1,10)  \
  X(2,1)  X(2,2)  X(2,5)  X(2,10)  \
  X(5,1)  X(5,2)  X(5,5)  X(5,10)  \
  X(10,1) X(10,2) X(10,5) X(10,10)

#define PHODS_DSE_RUNNER(bx, by) PHODS_RUNNER(phods_dse_##bx##_##by, bx, by)
PHODS_DSE_LIST(PHODS_DSE_RUNNER)

typedef struct
{
  const char *name;
  int         question;   /* 2, 3 or 5 (Q4 = the BX == BY rows of Q5) */
  int         bx, by;
  uint32_t  (*run)(void);
  const int  *vx;          /* motion vectors produced by the last run */
  const int  *vy;
} phods_config_t;

#define PHODS_ENTRY(q, fn, BX, BY) \
  { #fn, q, BX, BY, fn##_run, &fn##_vx[0][0], &fn##_vy[0][0] },
#define PHODS_DSE_ENTRY(bx, by) PHODS_ENTRY(5, phods_dse_##bx##_##by, bx, by)

static const phods_config_t phods_configs[] =
{
  PHODS_ENTRY(2, phods_v0_original, B, B)
  PHODS_ENTRY(3, phods_v1_hoist,    B, B)
  PHODS_ENTRY(3, phods_v2_bounds,   B, B)
  PHODS_ENTRY(3, phods_v3_share,    B, B)
  PHODS_ENTRY(3, phods_v4_copy,     B, B)
  PHODS_DSE_LIST(PHODS_DSE_ENTRY)
};

#define PHODS_NUM_CONFIGS ((int)(sizeof(phods_configs)/sizeof(phods_configs[0])))

#endif /* PHODS_RUNS_H_ */

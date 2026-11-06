/*
 * hal_entry.c - Lab 1, Ζητούμενο 1 (PHODS on the Renesas S7G2).
 *
 * Replace the hal_entry.c of a fresh "Blinky" Synergy project with this file
 * and copy the other files of this folder (images.h, phods_*.h) next to it.
 *
 * One debug session runs EVERY configuration PHODS_RUNS (=10) times:
 *   Q2  phods_v0_original                     (original code)
 *   Q3  phods_v1_hoist .. phods_v4_copy        (one row per transformation)
 *   Q4  phods_dse_<b>_<b>, b in {1,2,5,10}     (square blocks)
 *   Q5  phods_dse_<bx>_<by>, all 16 pairs      (rectangular blocks)
 *
 * When it finishes it stops at the breakpoint instruction near the end
 * (or put a breakpoint on the "while(1)" line). Then look at, in the
 * Expressions view:
 *   g_core_clock_hz        SystemCoreClock (Q1)
 *   g_results              one row per configuration (cycles + microseconds)
 *   g_all_correct          1 if every 5x5 variant matched the original vectors
 */

#include "hal_data.h"
#include <stdint.h>
#include "images.h"

/* cycle counter used by the runners in phods_runs.h */
#define PHODS_CYCLES() (DWT->CYCCNT)

#include "phods_runs.h"   /* includes phods_config.h (N, M, B) - keep it after hal_data.h */

typedef struct
{
  const char *name;
  int         question;
  int         bx, by;
  uint32_t    cycles[PHODS_RUNS];
  uint32_t    min_cycles, max_cycles;
  float       avg_cycles;
  float       min_us, max_us, avg_us;
  int         correct;   /* 1 ok, 0 WRONG, -1 not checked on board (not 5x5) */
} phods_result_t;

volatile uint32_t       g_core_clock_hz;
volatile int            g_all_correct;
volatile int            g_done;
volatile phods_result_t g_results[PHODS_NUM_CONFIGS];

static int same_vectors(const phods_config_t *a, const phods_config_t *b)
{
  int i, n = (N/a->bx) * (M/a->by);
  for(i = 0; i < n; i++)
  {
    if(a->vx[i] != b->vx[i] || a->vy[i] != b->vy[i]) return 0;
  }
  return 1;
}

void hal_entry(void)
{
  int c, r;

  /* Code to initialize the DWT->CYCCNT register (from the exercise) */
  CoreDebug->DEMCR |= 0x01000000;
  ITM->LAR = 0xC5ACCE55;
  DWT->CYCCNT = 0;
  DWT->CTRL |= 1;

  g_core_clock_hz = SystemCoreClock;
  g_all_correct   = 1;
  g_done          = 0;

  for(c = 0; c < PHODS_NUM_CONFIGS; c++)
  {
    const phods_config_t *cfg = &phods_configs[c];
    volatile phods_result_t *res = &g_results[c];
    uint32_t cyc, sum = 0, mn = 0xFFFFFFFFu, mx = 0;
    float mhz = (float)g_core_clock_hz / 1.0e6f;

    for(r = 0; r < PHODS_RUNS; r++)
    {
      cyc = cfg->run();          /* timed with DWT->CYCCNT inside */
      res->cycles[r] = cyc;
      sum += cyc;
      if(cyc < mn) mn = cyc;
      if(cyc > mx) mx = cyc;
    }

    res->name       = cfg->name;
    res->question   = cfg->question;
    res->bx         = cfg->bx;
    res->by         = cfg->by;
    res->min_cycles = mn;
    res->max_cycles = mx;
    res->avg_cycles = (float)sum / PHODS_RUNS;
    res->min_us     = (float)mn / mhz;
    res->max_us     = (float)mx / mhz;
    res->avg_us     = res->avg_cycles / mhz;

    /* every 5x5 configuration must reproduce the original's motion vectors */
    if(cfg->bx == B && cfg->by == B)
    {
      res->correct = same_vectors(cfg, &phods_configs[0]);
      if(!res->correct) g_all_correct = 0;
    }
    else
    {
      res->correct = -1;
    }
  }

  g_done = 1;
  __BKPT(0);   /* halts here when running under the debugger: read g_results */

  while(1)
  {
  }
}

/*
 * host_test.c - correctness check on a normal PC (no board needed).
 *
 * Builds the exact same PHODS headers that go on the board and checks that
 * every variant / every (BX, BY) produces exactly the motion vectors of the
 * ORIGINAL algorithm (generalised here to rectangular blocks).
 *
 *   make -C test          (or: cc -std=c99 -Wall -Wextra -I../src host_test.c)
 */
#include <stdio.h>
#include <stdint.h>
#include "images.h"

#define PHODS_CYCLES() 0u
#include "phods_runs.h"

/* The original algorithm, line for line, with B replaced by runtime bx / by. */
static void reference(const int current[N][M], const int previous[N][M],
                      int bx, int by, int *vxo, int *vyo)
{
  int x, y, i, k, l, p1, p2, dist, S, min1, min2, bestx = 0, besty = 0;
  int nbx = N/bx, nby = M/by;

  for(i = 0; i < nbx*nby; i++) { vxo[i] = 0; vyo[i] = 0; }

  for(x = 0; x < nbx; x++)
    for(y = 0; y < nby; y++)
    {
      int *vx = &vxo[x*nby + y], *vy = &vyo[x*nby + y];
      for(S = 4; S > 0; S /= 2)
      {
        min1 = 255*bx*by;
        min2 = 255*bx*by;
        for(i = -S; i < S+1; i += S)
        {
          dist = 0;
          for(k = 0; k < bx; k++)
            for(l = 0; l < by; l++)
            {
              p1 = current[bx*x+k][by*y+l];
              if((bx*x + *vx + i + k) < 0 || (bx*x + *vx + i + k) > (N-1) ||
                 (by*y + *vy + l) < 0     || (by*y + *vy + l) > (M-1))
                p2 = 0;
              else
                p2 = previous[bx*x + *vx + i + k][by*y + *vy + l];
              dist += (p1-p2 > 0) ? p1-p2 : p2-p1;
            }
          if(dist < min1) { min1 = dist; bestx = i; }
        }
        for(i = -S; i < S+1; i += S)
        {
          dist = 0;
          for(k = 0; k < bx; k++)
            for(l = 0; l < by; l++)
            {
              p1 = current[bx*x+k][by*y+l];
              if((bx*x + *vx + k) < 0     || (bx*x + *vx + k) > (N-1) ||
                 (by*y + *vy + i + l) < 0 || (by*y + *vy + i + l) > (M-1))
                p2 = 0;
              else
                p2 = previous[bx*x + *vx + k][by*y + *vy + i + l];
              dist += (p1-p2 > 0) ? p1-p2 : p2-p1;
            }
          if(dist < min2) { min2 = dist; besty = i; }
        }
        *vx += bestx;
        *vy += besty;
      }
    }
}

static int check_frames(const char *label, const int (*cur)[M], const int (*prev)[M])
{
  int c, i, failures = 0;
  int rx[N*M], ry[N*M];

  for(c = 0; c < PHODS_NUM_CONFIGS; c++)
  {
    const phods_config_t *cfg = &phods_configs[c];
    int n = (N/cfg->bx) * (M/cfg->by), ok = 1, nonzero = 0;

    /* runners always use the real images; for other frames call the
       reference twice to at least exercise it, real check is below */
    if(cur != PHODS_CUR) break;

    cfg->run();
    reference(cur, prev, cfg->bx, cfg->by, rx, ry);
    for(i = 0; i < n; i++)
    {
      if(cfg->vx[i] != rx[i] || cfg->vy[i] != ry[i]) ok = 0;
      if(rx[i] || ry[i]) nonzero = 1;
    }
    printf("[%s] %-22s %2dx%-2d  %s%s\n", label, cfg->name, cfg->bx, cfg->by,
           ok ? "OK" : "MISMATCH", nonzero ? "" : "  (all vectors zero)");
    if(!ok) failures++;
  }
  return failures;
}

/* Extra stress test: random frames with large values, every DSE function. */
static int rnd_cur[N][M], rnd_prev[N][M];
static unsigned rng = 12345u;
static int rnd(void) { rng = rng*1103515245u + 12345u; return (int)((rng >> 16) % 256) - 128; }

#define STRESS_ONE(bx, by)                                                    \
  {                                                                           \
    static int vx[N/(bx)][M/(by)], vy[N/(bx)][M/(by)];                        \
    int rx2[N*M], ry2[N*M], ii, ok2 = 1;                                      \
    phods_dse_##bx##_##by((const int (*)[M])rnd_cur,                          \
                          (const int (*)[M])rnd_prev, vx, vy);                \
    reference((const int (*)[M])rnd_cur, (const int (*)[M])rnd_prev,          \
              bx, by, rx2, ry2);                                              \
    for(ii = 0; ii < (N/(bx))*(M/(by)); ii++)                                 \
      if((&vx[0][0])[ii] != rx2[ii] || (&vy[0][0])[ii] != ry2[ii]) ok2 = 0;   \
    if(!ok2) { printf("random trial %d: dse %dx%d MISMATCH\n", t, bx, by);    \
               fails++; }                                                     \
  }

int main(void)
{
  int fails = check_frames("images.h", PHODS_CUR, PHODS_PREV);
  int t, i, j;

  for(t = 0; t < 2000; t++)
  {
    /* random frames, sometimes a shifted copy so real motion exists */
    for(i = 0; i < N; i++)
      for(j = 0; j < M; j++)
        rnd_prev[i][j] = rnd();
    for(i = 0; i < N; i++)
      for(j = 0; j < M; j++)
      {
        int si = i + (t % 7) - 3, sj = j + (t / 7 % 7) - 3;
        rnd_cur[i][j] = (t & 1 && si >= 0 && si < N && sj >= 0 && sj < M)
                        ? rnd_prev[si][sj] : rnd();
      }
    PHODS_DSE_LIST(STRESS_ONE)
    {
      static int vx[N/B][M/B], vy[N/B][M/B], wx[N/B][M/B], wy[N/B][M/B];
      phods_v0_original((const int (*)[M])rnd_cur, (const int (*)[M])rnd_prev, vx, vy);
      phods_v1_hoist((const int (*)[M])rnd_cur, (const int (*)[M])rnd_prev, wx, wy);
      for(i = 0; i < (N/B)*(M/B); i++)
        if((&vx[0][0])[i] != (&wx[0][0])[i] || (&vy[0][0])[i] != (&wy[0][0])[i])
        { printf("random trial %d: v1 MISMATCH\n", t); fails++; break; }
      phods_v2_bounds((const int (*)[M])rnd_cur, (const int (*)[M])rnd_prev, wx, wy);
      for(i = 0; i < (N/B)*(M/B); i++)
        if((&vx[0][0])[i] != (&wx[0][0])[i] || (&vy[0][0])[i] != (&wy[0][0])[i])
        { printf("random trial %d: v2 MISMATCH\n", t); fails++; break; }
      phods_v4_copy((const int (*)[M])rnd_cur, (const int (*)[M])rnd_prev, wx, wy);
      for(i = 0; i < (N/B)*(M/B); i++)
        if((&vx[0][0])[i] != (&wx[0][0])[i] || (&vy[0][0])[i] != (&wy[0][0])[i])
        { printf("random trial %d: v4 MISMATCH\n", t); fails++; break; }
    }
  }
  printf("random stress: 2000 frame pairs x (16 DSE configs + v1/v2/v4)\n");

  printf(fails ? "\nFAILED (%d)\n" : "\nALL OK\n", fails);
  return fails != 0;
}

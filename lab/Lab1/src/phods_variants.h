/*
 * phods_variants.h - Q3: hand-written intermediate optimization step.
 *
 * v1 = loop-invariant code motion + scalar replacement:
 *   - B*x, B*y computed once per block instead of in every pixel iteration
 *   - vectors_x[x][y] / vectors_y[x][y] kept in locals (vx, vy) instead of
 *     re-reading the output arrays in every pixel iteration
 *   - the row bound check (depends only on k) is moved out of the l loop
 *   - pointer to the current row taken once per k
 *   - separate zero-initialisation loop removed (vectors written once at end)
 *
 * Later steps (bounds-check elimination, shared centre candidate, copying the
 * block to RAM) are produced from the template in phods_impl.h.
 */
#ifndef PHODS_VARIANTS_H_
#define PHODS_VARIANTS_H_

#include "phods_config.h"

static void phods_v1_hoist(const int current[N][M], const int previous[N][M],
        int vectors_x[N/B][M/B], int vectors_y[N/B][M/B])
{
  int x, y, i, k, l, S, d, dist, min1, min2, bestx, besty;
  int bx, by, vx, vy, row, col;
  const int *cp;

  for(x=0; x<N/B; x++)
  {
    bx = B*x;
    for(y=0; y<M/B; y++)
    {
      by = B*y;
      vx = 0;
      vy = 0;

      for(S=4; S>0; S>>=1)
      {
        min1 = 255*B*B;
        min2 = 255*B*B;
        bestx = 0;
        besty = 0;

        /* X dimension */
        for(i=-S; i<=S; i+=S)
        {
          dist = 0;
          for(k=0; k<B; k++)
          {
            cp  = &current[bx+k][by];
            row = bx + vx + i + k;
            if(row < 0 || row > N-1)
            {
              for(l=0; l<B; l++) { d = cp[l]; dist += (d < 0) ? -d : d; }
            }
            else
            {
              for(l=0; l<B; l++)
              {
                col = by + vy + l;
                d = (col < 0 || col > M-1) ? cp[l] : cp[l] - previous[row][col];
                dist += (d < 0) ? -d : d;
              }
            }
          }
          if(dist < min1) { min1 = dist; bestx = i; }
        }

        /* Y dimension */
        for(i=-S; i<=S; i+=S)
        {
          dist = 0;
          for(k=0; k<B; k++)
          {
            cp  = &current[bx+k][by];
            row = bx + vx + k;
            if(row < 0 || row > N-1)
            {
              for(l=0; l<B; l++) { d = cp[l]; dist += (d < 0) ? -d : d; }
            }
            else
            {
              for(l=0; l<B; l++)
              {
                col = by + vy + i + l;
                d = (col < 0 || col > M-1) ? cp[l] : cp[l] - previous[row][col];
                dist += (d < 0) ? -d : d;
              }
            }
          }
          if(dist < min2) { min2 = dist; besty = i; }
        }

        vx += bestx;
        vy += besty;
      }

      vectors_x[x][y] = vx;
      vectors_y[x][y] = vy;
    }
  }
}

#endif /* PHODS_VARIANTS_H_ */

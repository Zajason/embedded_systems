/*
 * phods_impl.h - "template" for the optimized PHODS (Q3 final, Q4, Q5).
 *
 * NO include guard on purpose: this file is included several times, each time
 * with different parameters, to generate one specialised function per
 * configuration (block sizes are compile-time constants in every copy, just
 * like B in the original code).
 *
 * Define before including:
 *   PHODS_FN            function name to generate
 *   PHODS_BX, PHODS_BY  block rows / columns (PHODS_BX | N, PHODS_BY | M)
 *   PHODS_SHARE_CENTER  1 = compute the (vx,vy) candidate once per step
 *   PHODS_COPY_BLOCK    1 = copy the current block from Flash into a RAM buffer
 *
 * Generated:
 *   void PHODS_FN(const int cur[N][M], const int prev[N][M],
 *                 int vx[N/PHODS_BX][M/PHODS_BY], int vy[N/PHODS_BX][M/PHODS_BY]);
 *
 * Optimizations compared to the original:
 *   1. Loop-invariant code motion / scalar replacement (same as v1).
 *   2. Bounds-check elimination: the valid row range [kmin,kmax) and column
 *      range [lmin,lmax) of a candidate are computed ONCE per candidate, so
 *      the innermost loop has no if at all. Out-of-frame pixels contribute
 *      |cur| (p2 = 0); whole out-of-frame rows use a precomputed row sum.
 *   3. Common sub-expression / data reuse: the candidate i = 0 is the same
 *      block (vx,vy) in both the X and the Y search, so it is computed once
 *      (5 SADs per step instead of 6).        [PHODS_SHARE_CENTER]
 *   4. Copy the current block (const -> Flash) into a local buffer (RAM)
 *      once per block, because it is read 3 steps x 5-6 SADs times.
 *                                              [PHODS_COPY_BLOCK]
 * Results (motion vectors) are bit-identical to the original; this is checked
 * by test/host_test.c.
 */

#ifndef PHODS_PASTE
#define PHODS_PASTE_(a, b) a##b
#define PHODS_PASTE(a, b)  PHODS_PASTE_(a, b)
#endif

#define PHODS_SAD PHODS_PASTE(PHODS_FN, _sad)

/* SAD between the current block (cb, row stride cs, per-row |.| sums in
 * rowabs) and the block of prev whose top-left corner is (r, c).            */
static int PHODS_SAD(const int *cb, int cs, const int *rowabs,
                     const int prev[N][M], int r, int c)
{
  int k, l, d, sum = 0;
  int kmin, kmax, lmin, lmax;
  const int *cp;
  const int *pp;

  kmin = (r < 0) ? -r : 0;
  if(kmin > PHODS_BX) kmin = PHODS_BX;
  kmax = N - r;
  if(kmax > PHODS_BX) kmax = PHODS_BX;
  if(kmax < kmin)     kmax = kmin;

  lmin = (c < 0) ? -c : 0;
  if(lmin > PHODS_BY) lmin = PHODS_BY;
  lmax = M - c;
  if(lmax > PHODS_BY) lmax = PHODS_BY;
  if(lmax < lmin)     lmax = lmin;

  /* no column inside the frame: every row behaves like an out-of-frame row */
  if(lmin == lmax)    kmax = kmin;

  /* rows completely outside the frame: p2 = 0 for the whole row */
  for(k=0; k<kmin; k++)        sum += rowabs[k];
  for(k=kmax; k<PHODS_BX; k++) sum += rowabs[k];

  for(k=kmin; k<kmax; k++)
  {
    cp = cb + k*cs;
    pp = &prev[r+k][c+lmin];   /* first in-frame pixel of this row */

    for(l=0; l<lmin; l++)        { d = cp[l];         sum += (d < 0) ? -d : d; }
    for(l=lmin; l<lmax; l++)     { d = cp[l] - *pp++; sum += (d < 0) ? -d : d; }
    for(l=lmax; l<PHODS_BY; l++) { d = cp[l];         sum += (d < 0) ? -d : d; }
  }
  return sum;
}

static void PHODS_FN(const int cur[N][M], const int prev[N][M],
                     int vectors_x[N/PHODS_BX][M/PHODS_BY],
                     int vectors_y[N/PHODS_BX][M/PHODS_BY])
{
  int x, y, k, l, S, d, min1, min2, bestx, besty, vx, vy, r0, c0, cs;
  int rowabs[PHODS_BX];
  const int *cb;
#if PHODS_SHARE_CENTER
  int dc;
#endif
#if PHODS_COPY_BLOCK
  int blk[PHODS_BX*PHODS_BY];
#endif

  for(x=0; x<N/PHODS_BX; x++)
  {
    r0 = PHODS_BX*x;
    for(y=0; y<M/PHODS_BY; y++)
    {
      c0 = PHODS_BY*y;

#if PHODS_COPY_BLOCK
      for(k=0; k<PHODS_BX; k++)
        for(l=0; l<PHODS_BY; l++)
          blk[k*PHODS_BY + l] = cur[r0+k][c0+l];
      cb = blk;
      cs = PHODS_BY;
#else
      cb = &cur[r0][c0];
      cs = M;
#endif

      for(k=0; k<PHODS_BX; k++)
      {
        rowabs[k] = 0;
        for(l=0; l<PHODS_BY; l++)
        {
          d = cb[k*cs + l];
          rowabs[k] += (d < 0) ? -d : d;
        }
      }

      vx = 0;
      vy = 0;
      for(S=4; S>0; S>>=1)
      {
        min1 = 255*PHODS_BX*PHODS_BY;
        min2 = 255*PHODS_BX*PHODS_BY;
        bestx = 0;
        besty = 0;

#if PHODS_SHARE_CENTER
        dc = PHODS_SAD(cb, cs, rowabs, prev, r0+vx, c0+vy);
#endif
        /* X dimension: i = -S, 0, +S (same order / tie-breaking as original) */
        d = PHODS_SAD(cb, cs, rowabs, prev, r0+vx-S, c0+vy);
        if(d < min1) { min1 = d; bestx = -S; }
#if PHODS_SHARE_CENTER
        d = dc;
#else
        d = PHODS_SAD(cb, cs, rowabs, prev, r0+vx, c0+vy);
#endif
        if(d < min1) { min1 = d; bestx = 0; }
        d = PHODS_SAD(cb, cs, rowabs, prev, r0+vx+S, c0+vy);
        if(d < min1) { min1 = d; bestx = S; }

        /* Y dimension */
        d = PHODS_SAD(cb, cs, rowabs, prev, r0+vx, c0+vy-S);
        if(d < min2) { min2 = d; besty = -S; }
#if PHODS_SHARE_CENTER
        d = dc;
#else
        d = PHODS_SAD(cb, cs, rowabs, prev, r0+vx, c0+vy);
#endif
        if(d < min2) { min2 = d; besty = 0; }
        d = PHODS_SAD(cb, cs, rowabs, prev, r0+vx, c0+vy+S);
        if(d < min2) { min2 = d; besty = S; }

        vx += bestx;
        vy += besty;
      }

      vectors_x[x][y] = vx;
      vectors_y[x][y] = vy;
    }
  }
}

#undef PHODS_SAD
#undef PHODS_FN
#undef PHODS_BX
#undef PHODS_BY
#undef PHODS_SHARE_CENTER
#undef PHODS_COPY_BLOCK

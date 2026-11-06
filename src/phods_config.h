/*
 * phods_config.h - shared problem dimensions.
 *
 * Include AFTER hal_data.h: N, M and B are single-letter macros and must not
 * leak into the Renesas/CMSIS headers.
 *
 * The original skeleton also had "#define p 7" (search space) which is never
 * used by PHODS (the step sizes 4,2,1 implicitly give a +-7 search range).
 * It is left out on purpose: a macro called "p" breaks any later code that
 * uses p as an identifier.
 */
#ifndef PHODS_CONFIG_H_
#define PHODS_CONFIG_H_

#define N 10    /* frame rows    (as given in the exercise skeleton) */
#define M 10    /* frame columns (as given in the exercise skeleton) */
#define B 5     /* square block size used in Q2 / Q3 */

#define PHODS_RUNS 10   /* every configuration is executed this many times */

#endif /* PHODS_CONFIG_H_ */

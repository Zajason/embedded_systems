#include "hal_data.h"
#include "images.h"
#define current2 current
#define previous2 previous
#define hal_entry their_hal_entry
stub_cd_t stub_cd; stub_itm_t stub_itm; stub_dwt_t stub_dwt;
#include THEIR_FILE
int cur1[N][M], prev1[N][M], mvx[10][10], mvy[10][10];
void setup(void){ convert1DTo2D(previous, prev1); convert1DTo2D(current, cur1); }
void bench(void){ phods_motion_estimation(cur1, prev1, (void*)mvx, (void*)mvy); }
int abs(int v){ return v < 0 ? -v : v; }

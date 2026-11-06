#pragma once
#include <stdint.h>
typedef struct { volatile uint32_t DEMCR; } stub_cd_t;
typedef struct { volatile uint32_t LAR; } stub_itm_t;
typedef struct { volatile uint32_t CTRL, CYCCNT; } stub_dwt_t;
extern stub_cd_t stub_cd; extern stub_itm_t stub_itm; extern stub_dwt_t stub_dwt;
#define CoreDebug (&stub_cd)
#define ITM (&stub_itm)
#define DWT (&stub_dwt)

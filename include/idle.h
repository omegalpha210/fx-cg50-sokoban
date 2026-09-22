#ifndef SOK_IDLE_H
#define SOK_IDLE_H
#include <stdbool.h>
#include <stdint.h>
/* gint rtc_ticks() is a 128 Hz clock that wraps at midnight. */
#define SOK_CLOCK_HZ UINT32_C(128)
#define SOK_DAY_TICKS (UINT32_C(86400)*SOK_CLOCK_HZ)
typedef enum {SOK_IDLE_NONE,SOK_IDLE_DIM,SOK_IDLE_RESTORE,SOK_IDLE_OFF} SokIdleAction;
typedef struct {
    uint32_t last_activity,dim_after,off_after;
    bool dimmed;
} SokIdle;
void sok_idle_init(SokIdle *idle,uint32_t now,int off_minutes,int dim_half_minutes);
SokIdleAction sok_idle_update(SokIdle *idle,uint32_t now,bool active);
#endif

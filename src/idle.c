#include "idle.h"
void sok_idle_init(SokIdle *idle,uint32_t now,int off_minutes,int dim_half_minutes)
{
    /* Accept the CG50 SYSTEM choices, with finite defaults for invalid reads. */
    if(off_minutes!=10 && off_minutes!=60)off_minutes=10;
    if(dim_half_minutes!=1 && dim_half_minutes!=2 && dim_half_minutes!=6)dim_half_minutes=1;
    *idle=(SokIdle){now%SOK_DAY_TICKS,(uint32_t)dim_half_minutes*30u*SOK_CLOCK_HZ,
        (uint32_t)off_minutes*60u*SOK_CLOCK_HZ,false};
}
SokIdleAction sok_idle_update(SokIdle *idle,uint32_t now,bool active)
{
    now%=SOK_DAY_TICKS;
    if(active) {
        idle->last_activity=now;
        if(!idle->dimmed)return SOK_IDLE_NONE;
        idle->dimmed=false;return SOK_IDLE_RESTORE;
    }
    uint32_t elapsed=(now+SOK_DAY_TICKS-idle->last_activity)%SOK_DAY_TICKS;
    if(elapsed>=idle->off_after)return SOK_IDLE_OFF;
    if(elapsed>=idle->dim_after && !idle->dimmed) {
        idle->dimmed=true;return SOK_IDLE_DIM;
    }
    return SOK_IDLE_NONE;
}

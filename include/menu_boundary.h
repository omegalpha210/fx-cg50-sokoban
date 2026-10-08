#ifndef CG_MENU_BOUNDARY_H
#define CG_MENU_BOUNDARY_H
#include <stdint.h>
#include <stdbool.h>
#include <gint/drivers/keydev.h>

/* Project-local input ownership gate. This reads documented gint keydev state,
   never OS RAM. keydown()/keydev_idle() alone only see consumed-event state.
   The watchdog cancels a request; it never forces an unsafe OS handoff. */
enum { CG_MENU_IDLE, CG_MENU_WAIT, CG_MENU_READY, CG_MENU_INVALID, CG_MENU_TIMEOUT };
#define CG_MENU_WATCHDOG_TICKS (2u * 128u)
#define CG_MENU_WATCHDOG_STEPS 512u
typedef struct { bool pending,quiet;uint32_t requested,quiet_scan;unsigned steps; } CgMenuBoundary;
static inline void cg_menu_cancel(CgMenuBoundary *gate)
{ *gate=(CgMenuBoundary){0}; }
static inline void cg_menu_request(CgMenuBoundary *gate,uint32_t now)
{ if(!gate->pending)*gate=(CgMenuBoundary){.pending=true,.requested=now}; }
static inline int cg_menu_step(CgMenuBoundary *gate,keydev_t const *device,
    uint32_t now,uint32_t day_ticks)
{
    if(!gate->pending)return CG_MENU_IDLE;
    gate->steps++;
    if(!device || !day_ticks || now>=day_ticks || gate->requested>=day_ticks)
        return CG_MENU_INVALID;
    uint32_t elapsed=(now+day_ticks-gate->requested)%day_ticks;
    keydev_t const volatile *d=device;
    uint32_t scan=d->time;
    int next=d->queue_next,end=d->queue_end;
    if(next<0 || next>=KEYBOARD_QUEUE_SIZE || end<0 || end>=KEYBOARD_QUEUE_SIZE)
        return CG_MENU_INVALID;
    bool quiet=next==end;
    for(unsigned row=0;row<12;row++)
        if(d->state_now[row] || d->state_queue[row])quiet=false;
    /* An interrupt while sampling cannot manufacture a quiet boundary. */
    if(scan!=d->time || next!=d->queue_next || end!=d->queue_end)quiet=false;
    /* A completed quiet/fresh boundary stays safe after a long legitimate
       checkpoint. Timeouts only cancel boundaries that are not yet ready. */
    if(quiet && gate->quiet && scan!=gate->quiet_scan)return CG_MENU_READY;
    if(elapsed>=CG_MENU_WATCHDOG_TICKS || gate->steps>=CG_MENU_WATCHDOG_STEPS)
        return CG_MENU_TIMEOUT;
    if(!quiet){gate->quiet=false;return CG_MENU_WAIT;}
    if(!gate->quiet){gate->quiet=true;gate->quiet_scan=scan;return CG_MENU_WAIT;}
    return scan!=gate->quiet_scan?CG_MENU_READY:CG_MENU_WAIT;
}
#endif

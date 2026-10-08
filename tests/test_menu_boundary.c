#include "menu_boundary.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    CgMenuBoundary gate={0};keydev_t device={0};
    cg_menu_request(&gate,0);device.state_now[8]=8;device.state_queue[8]=8;
    assert(cg_menu_step(&gate,&device,0,11059200u)==CG_MENU_WAIT);
    device.state_now[8]=0;device.queue_end=1;device.time++;
    assert(cg_menu_step(&gate,&device,1,11059200u)==CG_MENU_WAIT);
    device.state_queue[8]=0;device.queue_next=1;
    assert(cg_menu_step(&gate,&device,2,11059200u)==CG_MENU_WAIT);
    assert(cg_menu_step(&gate,&device,3,11059200u)==CG_MENU_WAIT);
    device.time++;assert(cg_menu_step(&gate,&device,4,11059200u)==CG_MENU_READY);
    /* A long successful checkpoint doesn't invalidate an already safe gate. */
    assert(cg_menu_step(&gate,&device,5u*128u,11059200u)==CG_MENU_READY);
    device.state_now[0]=1;assert(cg_menu_step(&gate,&device,5u*128u,11059200u)==CG_MENU_TIMEOUT);
    cg_menu_cancel(&gate);assert(cg_menu_step(&gate,&device,0,11059200u)==CG_MENU_IDLE);
    memset(&device,0,sizeof device);cg_menu_request(&gate,0);
    for(unsigned i=0;i<CG_MENU_WATCHDOG_STEPS-1;i++)assert(cg_menu_step(&gate,&device,0,11059200u)==CG_MENU_WAIT);
    assert(cg_menu_step(&gate,&device,0,11059200u)==CG_MENU_TIMEOUT);
    cg_menu_cancel(&gate);cg_menu_request(&gate,0);device.queue_end=-1;
    assert(cg_menu_step(&gate,&device,0,11059200u)==CG_MENU_INVALID);
    cg_menu_cancel(&gate);cg_menu_request(&gate,11059200u-128u);memset(&device,0,sizeof device);
    assert(cg_menu_step(&gate,&device,0,11059200u)==CG_MENU_WAIT);
    assert(cg_menu_step(&gate,&device,128u,11059200u)==CG_MENU_TIMEOUT);
    puts("MENU gate: scanner/event states, queue, fresh scan, checkpoint delay, frozen RTC/scan, invalid queue and midnight PASS; no OS retention model");
    return 0;
}

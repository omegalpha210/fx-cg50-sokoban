#include "app.h"
#include "render.h"
#include "idle.h"
#include "system_power.h"
#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/drivers/keydev.h>
#include <gint/gint.h>
#include <gint/rtc.h>
#include <gint/cpu.h>
/* Static application data, one gint VRAM, no per-move allocation. */
static SokApp app;
static SokIdle idle;
static SokPowerSettings power_settings;
static void reload_power(void)
{
    sok_system_power_settings(&power_settings);
    sok_idle_init(&idle,rtc_ticks(),power_settings.off_minutes,power_settings.dim_half_minutes);
}
static void restore_backlight(void)
{
    if(idle.dimmed && power_settings.brightness>=1 && power_settings.brightness<=5)
        sok_system_backlight(power_settings.brightness);
    idle.dimmed=false;
}
static const int native_keys[SK_COUNT]={KEY_UP,KEY_RIGHT,KEY_DOWN,KEY_LEFT,KEY_F1,KEY_F2,KEY_F5,KEY_F6,KEY_EXE,KEY_EXIT,KEY_MENU,KEY_1,KEY_2,KEY_3,KEY_4,KEY_SHIFT,KEY_ALPHA,KEY_ACON};
static bool save(void *context,SokProgress *progress)
{(void)context;return sok_storage_save(progress);}
static void os_menu(void *context)
{(void)context;restore_backlight();gint_osmenu();reload_power();}
static void power_off(void *context)
{(void)context;restore_backlight();gint_poweroff(true);reload_power();}
static int repeat(int key,int duration,int count)
{
    (void)duration;
    if(key!=KEY_UP && key!=KEY_RIGHT && key!=KEY_DOWN && key!=KEY_LEFT)return -1;
    return count==0 ? 500000:125000;
}
static void barrier(void)
{
    /* Consume queued input and block every physically held mapped key until
       release. This covers keys held across modal transitions and OS resume. */
    clearevents();
    sok_input_init(&app.input);
    for(unsigned i=0;i<SK_COUNT;i++)if(keydown(native_keys[i]))app.input.held|=UINT32_C(1)<<i;
    sok_input_barrier(&app.input);
}
int main(void)
{
    dsetvram(gint_vram,NULL);
    sok_app_init(&app,(SokHooks){.save=save,.os_menu=os_menu,.power_off=power_off});
    SokLoadResult loaded=sok_storage_load(&app.progress);
    if(loaded==SOK_LOAD_RECOVERED)sok_app_load_notice(&app,true);
    else if(loaded==SOK_LOAD_INVALID || loaded==SOK_LOAD_IO_ERROR)sok_app_load_notice(&app,false);
    keydev_set_transform(keydev_std(),(keydev_transform_t){KEYDEV_TR_REPEATS,repeat});
    reload_power();barrier();sok_render(&app);
    for(;;) {
        /* Raw transformed keydev events retain releases,
           and never auto-handle MENU/OFF. Our input layer tracks tap/held
           modifiers so the complete checkpoint precedes either OS action. */
        key_event_t event=keydev_read(keydev_std(),false,NULL);
        bool activity=event.type==KEYEV_DOWN || event.type==KEYEV_UP
            || event.type==KEYEV_HOLD || !keydev_idle(keydev_std(),0);
        SokIdleAction action=sok_idle_update(&idle,rtc_ticks(),activity);
        if(action==SOK_IDLE_DIM && power_settings.brightness>=1 && power_settings.brightness<=5)
            sok_system_backlight(0); /* CG50 inactivity level, below user level 1. */
        else if(action==SOK_IDLE_RESTORE && power_settings.brightness>=1 && power_settings.brightness<=5)
            sok_system_backlight(power_settings.brightness);
        else if(action==SOK_IDLE_OFF) {
            sok_app_power_off(&app);barrier();sok_render(&app);continue;
        }
        SokKey key=SK_NONE;
        for(unsigned i=0;i<SK_COUNT;i++)if(event.key==(unsigned)native_keys[i]){key=(SokKey)i;break;}
        SokEventType type;
        if(event.type==KEYEV_DOWN)type=SE_DOWN;
        else if(event.type==KEYEV_UP)type=SE_UP;
        else if(event.type==KEYEV_HOLD)type=SE_HOLD;
        else {
            /* The existing periodic keyboard scan wakes the CPU. No extra
               timer, busy spin, or filesystem work in an interrupt. A key
               arriving before sleep is serviced at the next scanner tick. */
            sleep();continue;
        }
        unsigned epoch=app.epoch;
        bool changed=sok_app_event(&app,key,type);
        if(app.epoch!=epoch)barrier();
        if(changed)sok_render(&app);
    }
}

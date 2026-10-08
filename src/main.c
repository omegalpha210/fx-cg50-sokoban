#include "app.h"
#include "render.h"
#include "idle.h"
#include "system_power.h"
#include "usb_lifecycle.h"
#include "usb_native.h"
#include "menu_boundary.h"
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
static UsbLifecycle usb;
static CgMenuBoundary menu_boundary;
static bool deferred_off;
static void reload_power(void)
{
    sok_system_power_settings(&power_settings);
#if !defined(SOK_TEST_POWER_GINT_H)
    /* KhiCAS Golden Rule: 5 minutes (300 seconds) auto-park on hardware */
    sok_idle_init(&idle,rtc_ticks(),5,power_settings.dim_half_minutes);
#else
    sok_idle_init(&idle,rtc_ticks(),power_settings.off_minutes,power_settings.dim_half_minutes);
#endif
}
#if !defined(SOK_TEST_POWER_GINT_H)
static void show_poweroff_notice(void)
{
    int box_w = 340, box_h = 96;
    int box_x = (396 - box_w) / 2;
    int box_y = (224 - box_h) / 2;
    drect_border(box_x, box_y, box_x + box_w - 1, box_y + box_h - 1, C_WHITE, 2, C_RGB(0, 16, 31));
    dtext_opt(396 / 2, box_y + 16, C_BLACK, C_NONE, DTEXT_CENTER, DTEXT_TOP, "Back to Main Menu");
    dtext_opt(396 / 2, box_y + 46, C_RGB(0, 12, 28), C_NONE, DTEXT_CENTER, DTEXT_TOP, "To shutdown, press SHIFT AC/ON");
    dtext_opt(396 / 2, box_y + 66, C_DARK, C_NONE, DTEXT_CENTER, DTEXT_TOP, "again in Main Menu");
    dupdate();
}
#endif
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
{
    (void)context;app.menu_input_error=false;
    cg_menu_request(&menu_boundary,rtc_ticks());
}
static bool service_menu(void)
{
    int boundary=cg_menu_step(&menu_boundary,keydev_std(),rtc_ticks(),SOK_DAY_TICKS);
    if(boundary==CG_MENU_IDLE || boundary==CG_MENU_WAIT)return false;
    if(boundary!=CG_MENU_READY){cg_menu_cancel(&menu_boundary);app.menu_input_error=true;return true;}
    if(!usb_handoff_begin(&usb,usb_native_sample()))return false;
    if(!sok_storage_cleanup()){
        app.return_modal=app.modal;app.modal=SM_SAVE_ERROR;app.pending=SA_OS_MENU;
        cg_menu_cancel(&menu_boundary);usb_handoff_end(&usb,usb_native_sample());return true;
    }
    /* Filesystem world return can produce new input; recheck without clearing
       its events. The saved game cannot change while the request is deferred. */
    boundary=cg_menu_step(&menu_boundary,keydev_std(),rtc_ticks(),SOK_DAY_TICKS);
    if(boundary!=CG_MENU_READY){
        usb_handoff_end(&usb,usb_native_sample());
        if(boundary==CG_MENU_INVALID || boundary==CG_MENU_TIMEOUT){cg_menu_cancel(&menu_boundary);app.menu_input_error=true;return true;}
        return false;
    }
    cg_menu_cancel(&menu_boundary); /* Consume before the one OS call. */
#if !defined(SOK_TEST_POWER_GINT_H)
    while (keydown(KEY_MENU) || keydown(KEY_EXIT)) sleep();
    clearevents();
    sok_system_enable_menu_return();
#endif
    restore_backlight();gint_osmenu();reload_power();usb_handoff_end(&usb,usb_native_sample());
    /* Do not discard a genuinely new MENU/OFF queued after helper return. */
    sok_input_barrier(&app.input);return true;
}
static void power_off(void *context)
{
    (void)context;if(!usb_handoff_begin(&usb,usb_native_sample()))return;
    if(!sok_storage_cleanup()){
        app.return_modal=app.modal;app.modal=SM_SAVE_ERROR;app.pending=SA_STAY;
        usb_handoff_end(&usb,usb_native_sample());return;
    }
#if !defined(SOK_TEST_POWER_GINT_H)
    /* KhiCAS Rule: Display notice, wait 1 second (128 ticks), clear events,
       and safely park in Casio OS Main Menu via 0x1EA6 + gint_osmenu(). */
    show_poweroff_notice();
    uint32_t notice_t0 = rtc_ticks();
    while ((rtc_ticks() + SOK_DAY_TICKS - notice_t0) % SOK_DAY_TICKS < 128) sleep();
    clearevents();
    sok_system_enable_menu_return();
    restore_backlight();
    gint_osmenu();
    reload_power();
    usb_handoff_end(&usb,usb_native_sample());
    sok_input_barrier(&app.input);
#else
    restore_backlight();gint_poweroff(true);reload_power();usb_handoff_end(&usb,usb_native_sample());
#endif
}
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
static void collect_menu_event(key_event_t event)
{
    SokKey key=SK_NONE;
    for(unsigned i=0;i<SK_COUNT;i++)if(event.key==(unsigned)native_keys[i]){key=(SokKey)i;break;}
    SokEventType type;
    if(event.type==KEYEV_DOWN)type=SE_DOWN;
    else if(event.type==KEYEV_UP)type=SE_UP;
    else if(event.type==KEYEV_HOLD)type=SE_HOLD;
    else return;
    bool accepted=sok_input_event(&app.input,key,type);
    if(!accepted)return;
    if(app.input.poweroff)deferred_off=true;
    else if(key==SK_MENU)cg_menu_request(&menu_boundary,rtc_ticks());
    else if(key==SK_EXIT){cg_menu_cancel(&menu_boundary);app.menu_input_error=false;}
}
int main(void)
{
    cg_menu_cancel(&menu_boundary);deferred_off=false;
    dsetvram(gint_vram,NULL);
    sok_app_init(&app,(SokHooks){.save=save,.os_menu=os_menu,.power_off=power_off});
    SokLoadResult loaded=sok_storage_load(&app.progress);
    if(loaded==SOK_LOAD_RECOVERED)sok_app_load_notice(&app,true);
    else if(loaded==SOK_LOAD_INVALID || loaded==SOK_LOAD_IO_ERROR)sok_app_load_notice(&app,false);
    keydev_set_transform(keydev_std(),(keydev_transform_t){KEYDEV_TR_REPEATS,repeat});
    reload_power();usb_initialize(&usb,usb_native_sample());barrier();sok_render(&app);
    for(;;) {
        if(deferred_off && !menu_boundary.pending){
            deferred_off=false;sok_app_power_off(&app);sok_input_barrier(&app.input);sok_render(&app);continue;
        }
        /* Raw transformed keydev events retain releases,
           and never auto-handle MENU/OFF. Our input layer tracks tap/held
           modifiers so the complete checkpoint precedes either OS action. */
        key_event_t event=keydev_read(keydev_std(),false,NULL);
        usb_observe(&usb,usb_native_sample());
        if(usb_take_request(&usb)) {
            /* A detected insertion absorbs simultaneous MENU/OFF/APO input.
               One finite checkpoint attempt; existing save-error UI owns retry. */
            if(!menu_boundary.pending)(void)sok_app_key(&app,SK_MENU);
            if(!menu_boundary.pending){barrier();sok_render(&app);continue;}
            sok_render(&app);
        }
        if(menu_boundary.pending){
            /* No blocking release wait and no gameplay during an outstanding
               checkpointed MENU. Consume UPs and retain fresh global input. */
            for(unsigned count=0;count<32;count++){
                collect_menu_event(event);usb_observe(&usb,usb_native_sample());
                (void)usb_take_request(&usb); /* Coalesce into the existing MENU. */
                bool activity=event.type!=KEYEV_NONE || !keydev_idle(keydev_std(),0);
                SokIdleAction pending_idle=sok_idle_update(&idle,rtc_ticks(),activity);
                if(pending_idle==SOK_IDLE_OFF)deferred_off=true;
                if(pending_idle==SOK_IDLE_DIM && power_settings.brightness>=1 && power_settings.brightness<=5)
                    sok_system_backlight(0);
                else if(pending_idle==SOK_IDLE_RESTORE && power_settings.brightness>=1 && power_settings.brightness<=5)
                    sok_system_backlight(power_settings.brightness);
                if(event.type==KEYEV_NONE || count+1==32)break;
                event=keydev_read(keydev_std(),false,NULL);
            }
            if(service_menu() || !menu_boundary.pending)sok_render(&app);
            if(event.type==KEYEV_NONE)sleep();
            continue;
        }
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
        if(type==SE_DOWN && (key==SK_MENU || key==SK_ACON)){
            /* Also settle an insertion during a failed checkpoint, for which
               the OS callback was never reached. */
            (void)usb_handoff_begin(&usb,usb_native_sample());
            usb_handoff_end(&usb,usb_native_sample());
        }
        if(app.epoch!=epoch && !menu_boundary.pending)barrier();
        if(changed)sok_render(&app);
    }
}

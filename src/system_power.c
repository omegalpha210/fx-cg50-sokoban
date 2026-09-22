#include "system_power.h"
#include <gint/gint.h>
/* CG OS syscall facts and primary references are recorded in docs/POWER.md. */
extern int sok_os_auto_power_off(void);
extern char sok_os_backlight_duration(void);
extern char sok_os_light_level(void);
extern void sok_os_set_backlight(char level);
static int read_settings(void *context)
{
    SokPowerSettings *settings=context;
    settings->off_minutes=sok_os_auto_power_off();
    settings->dim_half_minutes=sok_os_backlight_duration();
    settings->brightness=sok_os_light_level();
    return 0;
}
static int set_brightness(void *context)
{
    sok_os_set_backlight((char)*(int *)context);return 0;
}
void sok_system_power_settings(SokPowerSettings *settings)
{gint_world_switch(GINT_CALL(read_settings,(void *)settings));}
void sok_system_backlight(int level)
{
    /* This add-in targets CG50: level 0 is its inactivity brightness. */
    if(level>=0 && level<=5)gint_world_switch(GINT_CALL(set_brightness,&level));
}

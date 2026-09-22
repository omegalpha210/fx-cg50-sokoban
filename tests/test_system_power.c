#include "system_power.h"
#include <gint/gint.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
static bool os_world;
static unsigned switches,reads,writes;
static int brightness;
int gint_world_switch(gint_call_t call)
{
    assert(!os_world);os_world=true;switches++;
    int result=call.function(call.argument);os_world=false;return result;
}
int sok_os_auto_power_off(void){assert(os_world);reads++;return 60;}
char sok_os_backlight_duration(void){assert(os_world);reads++;return 6;}
char sok_os_light_level(void){assert(os_world);reads++;return 4;}
void sok_os_set_backlight(char level)
{assert(os_world && level>=0 && level<=5);writes++;brightness=level;}
int main(void)
{
    SokPowerSettings settings;sok_system_power_settings(&settings);
    assert(!os_world && switches==1 && reads==3 && !writes);
    assert(settings.off_minutes==60 && settings.dim_half_minutes==6 && settings.brightness==4);
    for(int level=0;level<=5;level++){sok_system_backlight(level);assert(brightness==level);}
    sok_system_backlight(-1);sok_system_backlight(6);
    assert(writes==6 && switches==7 && !os_world);
    puts("system power: settings read and transient brightness changes stay in the OS world");
    return 0;
}

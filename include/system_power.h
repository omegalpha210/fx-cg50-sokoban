#ifndef SOK_SYSTEM_POWER_H
#define SOK_SYSTEM_POWER_H
typedef struct {int off_minutes,dim_half_minutes,brightness;} SokPowerSettings;
/* Main-thread OS-world adapters; never change saved SYSTEM preferences. */
void sok_system_power_settings(SokPowerSettings *settings);
void sok_system_backlight(int level); /* CG50: idle 0, normal 1..5. */
#endif

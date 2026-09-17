#ifndef SOK_TEST_KEYBOARD_H
#define SOK_TEST_KEYBOARD_H
#include <stdbool.h>
/* Matrix values checked against the installed gint 2.11.0 keycodes.h. */
enum {
    KEY_UP=0x86,KEY_RIGHT=0x76,KEY_DOWN=0x75,KEY_LEFT=0x85,
    KEY_F1=0x91,KEY_F2=0x92,KEY_F5=0x95,KEY_F6=0x96,
    KEY_EXE=0x15,KEY_EXIT=0x74,KEY_MENU=0x84,
    KEY_1=0x21,KEY_2=0x22,KEY_3=0x23,KEY_4=0x31,
    KEY_SHIFT=0x81,KEY_ALPHA=0x71,KEY_ACON=0x07,KEY_0=0x11
};
enum {KEYEV_NONE,KEYEV_DOWN,KEYEV_UP,KEYEV_HOLD};
typedef struct {unsigned type,key;} key_event_t;
void clearevents(void);
bool keydown(int key);
#endif

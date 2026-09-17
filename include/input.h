#ifndef SOK_INPUT_H
#define SOK_INPUT_H
#include <stdbool.h>
#include <stdint.h>
typedef enum {SK_UP,SK_RIGHT,SK_DOWN,SK_LEFT,SK_F1,SK_F2,SK_F5,SK_F6,SK_EXE,SK_EXIT,SK_MENU,SK_1,SK_2,SK_3,SK_4,SK_SHIFT,SK_ALPHA,SK_ACON,SK_COUNT,SK_NONE=-1} SokKey;
typedef enum {SE_DOWN,SE_UP,SE_HOLD} SokEventType;
typedef struct {
    uint32_t held,blocked;
    int direction;
    bool shift_pending,alpha_pending,poweroff;
} SokInput;
void sok_input_init(SokInput *in);
void sok_input_barrier(SokInput *in);
bool sok_input_event(SokInput *in,SokKey key,SokEventType type);
#endif

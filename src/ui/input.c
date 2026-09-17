#include "input.h"
void sok_input_init(SokInput *in) {*in=(SokInput){.direction=-1};}
void sok_input_barrier(SokInput *in)
{
    in->blocked|=in->held;in->direction=-1;
    in->shift_pending=false;in->alpha_pending=false;in->poweroff=false;
}
bool sok_input_event(SokInput *in,SokKey key,SokEventType type)
{
    in->poweroff=false;
    if(key<0 || key>=SK_COUNT) {
        /* An unmapped key still consumes a tapped modifier. */
        if(type==SE_DOWN){in->shift_pending=false;in->alpha_pending=false;}
        return false;
    }
    uint32_t bit=UINT32_C(1)<<(unsigned)key;
    if(type==SE_UP) {
        in->held&=~bit;in->blocked&=~bit;
        if(in->direction==(int)key)in->direction=-1;
        return false;
    }
    if(type!=SE_DOWN && type!=SE_HOLD)return false;
    bool already=(in->held&bit)!=0;
    /* gint can enqueue a final due repeat just after a release event.
       It must neither move nor mark that released key as held again. */
    if(type==SE_HOLD && !already)return false;
    in->held|=bit;
    if(in->blocked&bit)return false;
    if(type==SE_DOWN && already)return false;
    if(key==SK_SHIFT || key==SK_ALPHA) {
        if(type==SE_DOWN) {
            bool *pending=key==SK_SHIFT ? &in->shift_pending:&in->alpha_pending;
            *pending=!*pending;
        }
        return false;
    }
    if(type==SE_DOWN) {
        uint32_t modifiers=in->held & ~in->blocked;
        bool shift=in->shift_pending || (modifiers & (UINT32_C(1)<<SK_SHIFT));
        bool alpha=in->alpha_pending || (modifiers & (UINT32_C(1)<<SK_ALPHA));
        in->shift_pending=false;in->alpha_pending=false;
        in->poweroff=key==SK_ACON && shift && !alpha;
    }
    if(key==SK_ACON)return in->poweroff;
    if(key<=SK_LEFT) {
        if(type==SE_DOWN && in->direction<0)in->direction=key;
        return in->direction==(int)key;
    }
    return type==SE_DOWN;
}

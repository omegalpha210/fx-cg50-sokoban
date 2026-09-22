#include "idle.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    const int off[]={10,60},dim[]={1,2,6};
    const uint32_t starts[]={0,12345,SOK_DAY_TICKS-5u*SOK_CLOCK_HZ};
    unsigned cases=0;
    for(unsigned a=0;a<2;a++)for(unsigned b=0;b<3;b++)for(unsigned c=0;c<3;c++) {
        SokIdle idle;sok_idle_init(&idle,starts[c],off[a],dim[b]);
        uint32_t dim_at=(uint32_t)dim[b]*30u*SOK_CLOCK_HZ;
        uint32_t off_at=(uint32_t)off[a]*60u*SOK_CLOCK_HZ;
        for(uint32_t tick=0;tick<=off_at;tick++) {
            SokIdleAction expected=tick==dim_at ? SOK_IDLE_DIM:
                tick==off_at ? SOK_IDLE_OFF:SOK_IDLE_NONE;
            assert(sok_idle_update(&idle,(starts[c]+tick)%SOK_DAY_TICKS,false)==expected);
        }
        assert(sok_idle_update(&idle,starts[c]+off_at,true)==SOK_IDLE_RESTORE);
        assert(sok_idle_update(&idle,starts[c]+off_at+1,true)==SOK_IDLE_NONE);
        assert(sok_idle_update(&idle,starts[c]+off_at+dim_at,false)==SOK_IDLE_NONE);
        assert(sok_idle_update(&idle,starts[c]+off_at+dim_at+1,false)==SOK_IDLE_DIM);
        /* A held/unused key prevents both actions, even past the old deadline. */
        assert(sok_idle_update(&idle,starts[c]+off_at*2,true)==SOK_IDLE_RESTORE);
        assert(sok_idle_update(&idle,starts[c]+off_at*3,true)==SOK_IDLE_NONE);
        cases++;
    }
    SokIdle invalid;sok_idle_init(&invalid,0,-1,0);
    assert(invalid.off_after==600u*SOK_CLOCK_HZ && invalid.dim_after==30u*SOK_CLOCK_HZ);
    sok_idle_init(&invalid,0,2147483647,2147483647);
    assert(invalid.off_after==600u*SOK_CLOCK_HZ && invalid.dim_after==30u*SOK_CLOCK_HZ);
    printf("idle: %u full threshold sweeps, midnight wrap, activity and invalid settings passed\n",cases);
    return 0;
}

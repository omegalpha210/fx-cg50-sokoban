/* Shared actual renderer + an independently authored illustration puzzle. */
#include "render.h"
#include <gint/display.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
static SokApp app;
static const char *directory;
static void write_capture(const char *name)
{
    char path[512];snprintf(path,sizeof(path),"%s/%s.ppm",directory,name);
    FILE *f=fopen(path,"wb");assert(f);fprintf(f,"P6\n396 224\n255\n");
    for(unsigned i=0;i<396*224;i++) {
        unsigned p=host_pixels[i];
        unsigned char rgb[3]={(unsigned char)(((p>>11)&31u)*255u/31u),(unsigned char)(((p>>5)&63u)*255u/63u),(unsigned char)((p&31u)*255u/31u)};
        assert(fwrite(rgb,1,3,f)==3);
    }
    assert(!fclose(f));
}
static void capture(const char *name)
{
    host_out_of_bounds=0;sok_render(&app);assert(!host_out_of_bounds);
    write_capture(name);
}
static void markers(void)
{
    dclear(C_RGB(30,29,26));host_out_of_bounds=0;
    for(int size=9;size<=19;size++) {
        int x=8+(size-9)*35,y=30;
        sok_draw_player(x,y,size);
        unsigned blue=0,edge=0;
        for(int py=y-1;py<=y+size;py++)for(int px=x-1;px<=x+size;px++) {
            uint16_t color=host_pixels[py*396+px];
            if(color==C_RGB(0,17,31))blue++;
            if(color==C_RGB(0,5,15))edge++;
            if(px<=x || px>=x+size-1 || py<=y || py>=y+size-1)
                assert(color==C_RGB(30,29,26));
        }
        assert(blue>=10 && edge>=12); /* visibly larger than a 2x2 goal */
    }
    assert(!host_out_of_bounds);write_capture("player-sizes");
}
int main(int argc,char **argv)
{
    assert(argc==2);directory=argv[1];(void)mkdir(directory,0755);
    memset(&app,0,sizeof(app));app.screen=SOK_MAIN;capture("main");
    app.screen=SOK_LEVELS;app.selection=5;
    app.progress.cleared[0]=app.progress.cleared[2]=app.progress.cleared[4]=1;
    capture("level-select");
    app.screen=SOK_PLAY;app.level=1;assert(sok_init(sok_get_map(1),&app.game));
    capture("gameplay-original-fixture");
    app.modal=SM_INIT;capture("confirmation-original-fixture");
    const SokMap *m=sok_get_map(1);unsigned c=0;
    for(unsigned p=0;p<(unsigned)m->width*m->height;p++)
        if(sok_map_terrain(m,(uint16_t)p)==SOK_GOAL)app.game.crates[c++]=(uint16_t)p;
    assert(c==m->crate_count);app.game.moves=20;app.game.pushes=10;
    assert(sok_validate(m,&app.game));assert(sok_solved(m,&app.game));
    app.modal=SM_WIN;capture("completed-original-fixture");
    app.modal=SM_SAVE_ERROR;capture("save-error-original-fixture");
    markers();
    puts("Public renderer: six UI captures + player size 9..19 visibility/bounds checks; no upstream maps.");
    return 0;
}

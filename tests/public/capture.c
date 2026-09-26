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
    for(unsigned group=0;group<4;group++)for(int size=9;size<=19;size++) {
        int x=8+(size-9)*35,y=20+(int)group*50;
        sok_draw_player(x,y,size,group);
        unsigned fill=0,edge=0;
        for(int py=y-1;py<=y+size;py++)for(int px=x-1;px<=x+size;px++) {
            uint16_t color=host_pixels[py*396+px];
            if(color==sok_player_color(group))fill++;
            if(color!=sok_player_color(group) && color!=C_WHITE && color!=C_RGB(30,29,26))edge++;
            if(px<=x || px>=x+size-1 || py<=y || py>=y+size-1)
                assert(color==C_RGB(30,29,26));
        }
        assert(fill>=10 && edge>=12); /* visibly larger than a 2x2 goal */
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
    puts("Fixture renderer: UI captures + four player palettes at 9..19px; no upstream maps.");
    return 0;
}

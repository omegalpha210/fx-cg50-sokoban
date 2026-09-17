/* This executable calls the unmodified application renderer. Scripted fixtures
   are layout evidence, not a calculator emulator or proof of puzzle solving. */
#include "render.h"
#include <gint/display.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
static SokApp app;
static SokProgress restored;
static uint8_t encoded[SOK_SAVE_MAX_SIZE];
static const char *directory;
static unsigned captures;
static void tap(SokKey key)
{
    (void)sok_app_event(&app,key,SE_DOWN);
    (void)sok_app_event(&app,key,SE_UP);
}
static void capture(const char *name)
{
    host_out_of_bounds=0;sok_render(&app);assert(host_out_of_bounds==0);
    char path[1024];int n=snprintf(path,sizeof(path),"%s/%s.ppm",directory,name);
    assert(n>0 && (size_t)n<sizeof(path));FILE *f=fopen(path,"wb");assert(f);
    fprintf(f,"P6\n396 224\n255\n");
    for(unsigned i=0;i<396*224;i++) {
        uint16_t p=host_pixels[i];
        unsigned char rgb[3]={(unsigned char)(((p>>11)&31u)*255u/31u),(unsigned char)(((p>>5)&63u)*255u/63u),(unsigned char)((p&31u)*255u/31u)};
        assert(fwrite(rgb,1,3,f)==3);
    }
    assert(fclose(f)==0);captures++;
}
static void play(unsigned level)
{
    app.level=level;app.group=(level-1)/15;app.selection=(level-1)%15;
    app.screen=SOK_PLAY;app.modal=SM_NONE;assert(sok_init(sok_get_map(level),&app.game));
}
static void solved_fixture(void)
{
    const SokMap *map=sok_get_map(app.level);
    unsigned c=0;
    for(unsigned p=0;p<(unsigned)map->width*map->height;p++)
        if(sok_map_terrain(map,(uint16_t)p)==SOK_GOAL)app.game.crates[c++]=(uint16_t)p;
    assert(c==map->crate_count);
    for(unsigned p=0;p<(unsigned)map->width*map->height;p++)
        if(sok_map_terrain(map,(uint16_t)p)==SOK_FLOOR){app.game.player=(uint16_t)p;break;}
    app.game.moves=100;app.game.pushes=80;app.game.undo_count=0;
    memset(app.game.undo,0,sizeof(app.game.undo));
    assert(sok_validate(map,&app.game) && sok_solved(map,&app.game));
}
int main(int argc,char **argv)
{
    assert(argc==2);directory=argv[1];(void)mkdir(directory,0755);
    sok_app_init(&app,(SokHooks){0});
    assert(sok_text_width("INTERMEDIATE",1)*3/2<=171);
    capture("main");
    tap(SK_RIGHT);assert(app.group==1);capture("main-right-before-row");
    tap(SK_RIGHT);assert(app.group==2);capture("main-right-after-row");
    for(unsigned g=0;g<4;g++) {
        app.screen=SOK_LEVELS;app.group=g;app.selection=6;
        for(unsigned i=0;i<15;i++)app.progress.cleared[g*15+i]=(uint8_t)(i%3==0);
        char name[40];snprintf(name,sizeof(name),"levels-group-%u",g+1);capture(name);
        app.selection=4;
        snprintf(name,sizeof(name),"levels-%u-right-before-row",g+1);capture(name);
        tap(SK_RIGHT);assert(app.selection==5);
        snprintf(name,sizeof(name),"levels-%u-right-after-row",g+1);capture(name);
    }
    char layout_path[1024];
    int length=snprintf(layout_path,sizeof(layout_path),"%s/layout.csv",directory);
    assert(length>0 && (size_t)length<sizeof(layout_path));
    FILE *layout=fopen(layout_path,"w");assert(layout);
    assert(fprintf(layout,"level,x,y,tile,width,height\n")>0);
    int minimum=100,maximum=0;
    for(unsigned id=1;id<=60;id++) {
        play(id);SokBoardLayout b=sok_board_layout(sok_get_map(id));
        assert(b.x>=SOK_BOARD_LEFT && b.y>=SOK_PLAY_TOP &&
            b.x+b.width<=SOK_BOARD_LEFT+SOK_BOARD_WIDTH &&
            b.y+b.height<=SOK_PLAY_TOP+SOK_PLAY_HEIGHT && b.tile>=9);
        int left=b.x-SOK_BOARD_LEFT,right=SOK_BOARD_LEFT+SOK_BOARD_WIDTH-b.x-b.width;
        int top=b.y-SOK_PLAY_TOP,bottom=SOK_PLAY_TOP+SOK_PLAY_HEIGHT-b.y-b.height;
        assert(abs(left-right)<=1 && abs(top-bottom)<=1);
        assert(fprintf(layout,"%u,%d,%d,%d,%d,%d\n",id,b.x,b.y,b.tile,b.width,b.height)>0);
        if(b.tile<minimum)minimum=b.tile;
        if(b.tile>maximum)maximum=b.tile;
        char name[24];snprintf(name,sizeof(name),"play-%02u",id);capture(name);
        /* The gameplay title must remain absent; the top margin is all paper. */
        for(unsigned x=0;x<396;x++)assert(host_pixels[x]==C_RGB(29,30,30));
        /* HUD, divider, and board occupy distinct horizontal regions. */
        for(unsigned y=4;y<200;y++) {
            assert(host_pixels[y*396+118]==C_RGB(29,30,30));
            assert(host_pixels[y*396+122]==C_RGB(29,30,30));
        }
    }
    assert(fclose(layout)==0);
    play(12);capture("goals-and-crates");
    const SokMap *map=sok_get_map(12);
    for(unsigned p=0;p<(unsigned)map->width*map->height;p++) {
        if(sok_map_terrain(map,(uint16_t)p)==SOK_GOAL && sok_crate_at(map,&app.game,(uint16_t)p)<0) {
            app.game.player=(uint16_t)p;app.game.moves=1;break;
        }
    }
    assert(sok_validate(map,&app.game));capture("player-on-goal-fixture");
    app.modal=SM_INIT;capture("init-confirm");
    solved_fixture();app.modal=SM_WIN;capture("congratulations-fixture");
    play(60);solved_fixture();app.modal=SM_WIN;capture("level60-complete-fixture");
    app.modal=SM_SAVE_ERROR;capture("save-error");
    app.screen=SOK_MAIN;app.modal=SM_LOAD_NOTICE;capture("load-unavailable");
    app.recovered_notice=true;capture("backup-recovered");
    play(1);
    /* Capture real legal input, undo, then codec-backed restoration. */
    SokState original=app.game;bool moved=false;
    for(int d=0;d<4;d++)if(sok_move(sok_get_map(1),&app.game,(SokDirection)d)){moved=true;break;}
    assert(moved);capture("undo-before");
    assert(sok_progress_checkpoint(&app.progress,1,&app.game,false));
    size_t encoded_length=0;assert(sok_save_encode(&app.progress,1,encoded,sizeof(encoded),&encoded_length));
    assert(sok_save_decode(encoded,encoded_length,&restored));
    assert(sok_undo(sok_get_map(1),&app.game));assert(memcmp(&app.game,&original,sizeof(original))==0);
    capture("undo-after");
    assert(sok_progress_resume(&restored,1,&app.game));capture("progress-restored");
    app.game.moves=UINT32_MAX;app.game.pushes=UINT32_MAX;
    app.game.undo_count=0;memset(app.game.undo,0,sizeof(app.game.undo));capture("hud-max-counters-fixture");
    printf("Renderer: %u captures, all 60 boards fit; tiles %d..%d px; INTERMEDIATE width %d/187 px; no out-of-bounds draw.\n",captures,minimum,maximum,sok_text_width("INTERMEDIATE",1)*3/2);
    printf("sizeof SokState=%zu undo=%zu progress=%zu app=%zu; max encoded buffer=%u\n",sizeof(SokState),sizeof(app.game.undo),sizeof(SokProgress),sizeof(SokApp),SOK_SAVE_MAX_SIZE);
    return 0;
}

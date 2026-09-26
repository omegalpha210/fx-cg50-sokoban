/* README captures: the real app/renderer/map pack, with no terrain or state
 * substitution. Completion is reached by replaying legal moves from level 1.
 * This is host output, not a calculator photograph. */
#include "render.h"
#include <gint/display.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
static SokApp app;
static const char *directory;
static unsigned captures;
static bool save(void *context,SokProgress *progress)
{
    (void)context;++progress->generation;progress->dirty=false;return true;
}
static bool tap(SokKey key)
{
    bool changed=sok_app_event(&app,key,SE_DOWN);
    (void)sok_app_event(&app,key,SE_UP);return changed;
}
static void capture(const char *name)
{
    host_out_of_bounds=0;sok_render(&app);assert(!host_out_of_bounds);
    char path[512];int length=snprintf(path,sizeof(path),"%s/%s.ppm",directory,name);
    assert(length>0 && (size_t)length<sizeof(path));
    FILE *f=fopen(path,"wb");assert(f);fprintf(f,"P6\n396 224\n255\n");
    for(unsigned i=0;i<396*224;i++) {
        unsigned p=host_pixels[i];
        unsigned char rgb[3]={(unsigned char)(((p>>11)&31u)*255u/31u),
            (unsigned char)(((p>>5)&63u)*255u/63u),(unsigned char)((p&31u)*255u/31u)};
        assert(fwrite(rgb,1,3,f)==3);
    }
    assert(!fclose(f));captures++;
}
static void check_accent(unsigned group)
{
    /* Top stripe of the restart panel must use the group's player fill. */
    sok_render(&app);
    int y=SOK_PLAY_TOP+(SOK_PLAY_HEIGHT-88)/2+3;
    assert(host_pixels[y*396+198]==sok_player_color(group));
}
int main(int argc,char **argv)
{
    assert(argc==2);directory=argv[1];(void)mkdir(directory,0755);
    sok_app_init(&app,(SokHooks){.save=save});capture("main");
    const unsigned levels[]={1,16,31,59};
    const char *const groups[]={"basic","intermediate","advanced","master"};
    for(unsigned g=0;g<4;g++) {
        assert(tap((SokKey)(SK_1+g)));assert(app.screen==SOK_LEVELS);
        char name[64];snprintf(name,sizeof(name),"levels-%s",groups[g]);capture(name);
        for(unsigned i=g*15+1;i<levels[g];i++)assert(tap(SK_RIGHT));
        assert(tap(SK_EXE));assert(app.level==levels[g]);
        assert(sok_validate(sok_get_map(app.level),&app.game) && app.game.moves==0);
        snprintf(name,sizeof(name),"play-%s",groups[g]);capture(name);
        assert(tap(SK_F1));check_accent(g);assert(tap(SK_EXIT));
        assert(tap(SK_EXIT));assert(tap(SK_EXIT));assert(app.screen==SOK_MAIN);
    }
    assert(tap(SK_1));assert(tap(SK_EXE));assert(app.level==1 && app.game.moves==0);
    /* A project-generated, non-optimal solution. No map layout is embedded. */
    static const char route[]="ULLLUUULLULLDLLDDDRRLLUUURRURRDLRRRDDDRDDLLLLLUURRRRRRRRRRRRDRULLLLLLLLLLLLLLLULLDRRRRRRRRRRRRRRRRRRDRULLLURDLLLLLLLLLUUULLULUURDDUULLDRDDLDDDRRRRUUULLUULLDDDDUUURRDLULDDURRRRDDDLLLLLLULLDRRRRRRRRRDDLLLLLUDRRRRRUURRRRRRRURDLDRRULLLLLLLLLLLLLLLLULLDRRRRRRRRRRRRRRRRRRLLLLLLLLLLUUULLLLDDDLLUUURLDDDRRRRRRRDDLLLLLUDRRRRRUULUUULLULLDLLDDDRRRRRRRRRRRRRRRLLLLLLLLLUUULLLLDDUULLDDDRRRRRRRRRRRRRRURDLDR";
    static const char directions[]="URDL";
    for(unsigned i=0;route[i];i++) {
        const char *key=strchr(directions,route[i]);assert(key);
        assert(tap((SokKey)(key-directions)));
        assert(app.game.moves==i+1 && sok_validate(sok_get_map(1),&app.game));
        if(i==19) {
            SokState before=app.game;assert(tap(SK_F1));capture("restart-basic");
            assert(tap(SK_EXIT));assert(!memcmp(&before,&app.game,sizeof(before)));
        }
    }
    assert(app.modal==SM_WIN && sok_solved(sok_get_map(1),&app.game));
    assert(app.progress.cleared[0] && !app.progress.in_progress[0] && !app.progress.dirty);
    assert(app.game.moves==394 && app.game.pushes==134 && app.game.undo_count==5);
    capture("win-basic");assert(tap(SK_EXIT));
    assert(app.screen==SOK_PLAY && app.modal==SM_NONE);capture("completed-basic");
    assert(host_pixels[205*396+1]==C_RGB(24,26,26));
    assert(host_pixels[205*396+67]==C_RGB(24,26,26));
    assert(host_pixels[205*396+331]==C_RGB(3,5,7));
    SokState completed=app.game;
    for(int key=SK_UP;key<=SK_F2;key++)assert(!tap((SokKey)key));
    assert(!memcmp(&completed,&app.game,sizeof(completed)));
    assert(tap(SK_EXIT));assert(app.screen==SOK_LEVELS);
    capture("levels-basic"); /* actual earned completion marker */
    assert(tap(SK_EXE));assert(app.game.moves==0 && app.progress.cleared[0]);
    printf("README: %u captures of real levels 1/16/31/59; level 1 solved by 394 legal moves/134 pushes; completed view locked.\n",captures);
    return 0;
}

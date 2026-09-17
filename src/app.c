#include "app.h"
#include <string.h>
static void transition(SokApp *app) {app->epoch++;sok_input_barrier(&app->input);}
static void open_level(SokApp *app,unsigned level)
{
    if(level<1 || level>SOK_LEVEL_COUNT)return;
    app->level=level;app->group=(level-1)/15;app->selection=(level-1)%15;
    sok_progress_resume(&app->progress,level,&app->game);
    app->screen=SOK_PLAY;app->modal=SM_NONE;transition(app);
}
static void perform(SokApp *app)
{
    SokAction action=app->pending;
    app->modal=app->return_modal;
    if(action==SA_LEVEL_MENU) {
        app->screen=SOK_LEVELS;app->group=(app->level-1)/15;
        app->selection=(app->level-1)%15;app->modal=SM_NONE;
    } else if(action==SA_OPEN_LEVEL) {open_level(app,app->target_level);return;}
    else if(action==SA_WIN)app->modal=SM_WIN;
    else if(action==SA_OS_MENU && app->hooks.os_menu)app->hooks.os_menu(app->hooks.context);
    transition(app);
}
static bool save_now(SokApp *app)
{
    if(app->screen==SOK_PLAY && !sok_progress_checkpoint(&app->progress,
        app->level,&app->game,sok_solved(sok_get_map(app->level),&app->game)))return false;
    if(!app->progress.dirty)return true;
    return app->hooks.save && app->hooks.save(app->hooks.context,&app->progress);
}
static void checkpoint(SokApp *app,SokAction action)
{
    app->pending=action;app->return_modal=app->modal;
    if(save_now(app))perform(app);
    else {app->modal=SM_SAVE_ERROR;transition(app);}
}
static bool power_off(SokApp *app)
{
    /* Main-thread boundary: the synchronous two-slot transaction returns
       (including all closes/readback) before entering the OS power routine.
       A failed write must not trap the power key in the save-error modal. */
    app->power_save_failed=app->progress.dirty && !save_now(app);
    if(app->hooks.power_off)app->hooks.power_off(app->hooks.context);
    /* gint resumes here after ON. Preserve RAM/screen and require fresh keys. */
    transition(app);return true;
}
void sok_app_init(SokApp *app,SokHooks hooks)
{
    memset(app,0,sizeof(*app));app->hooks=hooks;app->screen=SOK_MAIN;
    sok_progress_init(&app->progress);sok_input_init(&app->input);
}
void sok_app_load_notice(SokApp *app,bool recovered)
{
    app->recovered_notice=recovered;app->modal=SM_LOAD_NOTICE;transition(app);
}
static bool selector(unsigned *index,unsigned cols,unsigned rows,SokKey key)
{
    unsigned count=cols*rows;
    if(key==SK_LEFT){*index=(*index+count-1)%count;return true;}
    if(key==SK_RIGHT){*index=(*index+1)%count;return true;}
    unsigned row=*index/cols,col=*index%cols;
    if(key==SK_UP)row=(row+rows-1)%rows;
    else if(key==SK_DOWN)row=(row+1)%rows;
    else return false;
    *index=row*cols+col;return true;
}
bool sok_app_key(SokApp *app,SokKey key)
{
    if(app->modal==SM_SAVE_ERROR) {
        if(key==SK_EXE) {
            if(save_now(app))perform(app);
            else transition(app);
            return true;
        }
        if(key==SK_F6) {perform(app);return true;}
        if(key==SK_EXIT) {
            /* A completed board always retains a way to leave the win screen. */
            app->modal=app->pending==SA_WIN ? SM_WIN:app->return_modal;
            transition(app);return true;
        }
        return false;
    }
    if(key==SK_MENU) {checkpoint(app,SA_OS_MENU);return true;}
    if(app->modal==SM_LOAD_NOTICE) {
        if(key==SK_EXE || key==SK_EXIT) {app->modal=SM_NONE;transition(app);return true;}
        return false;
    }
    if(app->modal==SM_INIT) {
        if(key==SK_EXIT){app->modal=SM_NONE;transition(app);return true;}
        if(key==SK_EXE) {
            sok_init(sok_get_map(app->level),&app->game);app->modal=SM_NONE;
            checkpoint(app,SA_STAY);return true;
        }
        return false;
    }
    if(app->modal==SM_WIN) {
        if(key==SK_EXIT || (key==SK_EXE && app->level==SOK_LEVEL_COUNT)) {
            checkpoint(app,SA_LEVEL_MENU);return true;
        }
        if(key==SK_EXE) {app->target_level=app->level+1;checkpoint(app,SA_OPEN_LEVEL);return true;}
        return false;
    }
    if(app->screen==SOK_MAIN) {
        if(selector(&app->group,2,2,key))return true;
        if(key>=SK_1 && key<=SK_4)app->group=(unsigned)(key-SK_1);
        else if(key!=SK_EXE && key!=SK_F6)return false;
        app->screen=SOK_LEVELS;app->selection=0;transition(app);return true;
    }
    if(app->screen==SOK_LEVELS) {
        if(selector(&app->selection,5,3,key))return true;
        if(key==SK_EXIT){app->screen=SOK_MAIN;transition(app);return true;}
        if(key==SK_EXE || key==SK_F6){open_level(app,app->group*15+app->selection+1);return true;}
        return false;
    }
    const SokMap *map=sok_get_map(app->level);
    if(key>=SK_UP && key<=SK_LEFT) {
        if(!sok_move(map,&app->game,(SokDirection)key))return false;
        app->progress.dirty=true;
        if(sok_solved(map,&app->game))checkpoint(app,SA_WIN);
        return true;
    }
    if(key==SK_F1){app->modal=SM_INIT;transition(app);return true;}
    if(key==SK_F2) {
        if(!sok_undo(map,&app->game))return false;
        app->progress.dirty=true;return true;
    }
    if(key==SK_EXIT){checkpoint(app,SA_LEVEL_MENU);return true;}
    if((key==SK_F5 && app->level>1) || (key==SK_F6 && app->level<SOK_LEVEL_COUNT)) {
        app->target_level=key==SK_F5 ? app->level-1:app->level+1;
        checkpoint(app,SA_OPEN_LEVEL);return true;
    }
    return false;
}
bool sok_app_event(SokApp *app,SokKey key,SokEventType type)
{
    if(!sok_input_event(&app->input,key,type))return false;
    if(app->input.poweroff)return power_off(app);
    return sok_app_key(app,key);
}

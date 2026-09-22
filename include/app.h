#ifndef SOK_APP_H
#define SOK_APP_H
#include "storage.h"
#include "input.h"
typedef enum {SOK_MAIN,SOK_LEVELS,SOK_PLAY} SokScreen;
typedef enum {SM_NONE,SM_INIT,SM_WIN,SM_SAVE_ERROR,SM_LOAD_NOTICE} SokModal;
typedef enum {SA_STAY,SA_LEVEL_MENU,SA_OPEN_LEVEL,SA_OS_MENU,SA_WIN} SokAction;
typedef struct {
    bool (*save)(void *context,SokProgress *progress);
    void (*os_menu)(void *context);
    void *context;
    void (*power_off)(void *context);
} SokHooks;
typedef struct {
    SokProgress progress;
    SokState game;
    SokHooks hooks;
    SokInput input;
    SokScreen screen;
    SokModal modal,return_modal;
    SokAction pending;
    unsigned group,selection,level,target_level,epoch;
    bool recovered_notice,power_save_failed;
} SokApp;
void sok_app_init(SokApp *app,SokHooks hooks);
void sok_app_load_notice(SokApp *app,bool recovered);
bool sok_app_event(SokApp *app,SokKey key,SokEventType type);
/* Shared manual/automatic OFF path, only from the main thread. */
bool sok_app_power_off(SokApp *app);
/* Direct key dispatch is also used for bounded workflow tests. */
bool sok_app_key(SokApp *app,SokKey key);
#endif

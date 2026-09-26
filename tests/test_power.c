/* Compile the actual native entry point, replacing only hardware callbacks.
 * The real input, workflow, save codec and two-slot transaction are linked. */
#define main sok_native_main
#include "../src/main.c"
#undef main
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

typedef struct {key_event_t event;void (*check)(void);uint32_t elapsed;} Step;
#define DOWN(key_) {{KEYEV_DOWN,key_},NULL,0}
#define UP(key_) {{KEYEV_UP,key_},NULL,0}
#define HOLD(key_) {{KEYEV_HOLD,key_},NULL,0}
#define CHECK(fn_) {{KEYEV_NONE,0},fn_,0}
#define WAIT(seconds_) {{KEYEV_NONE,0},NULL,(seconds_)*SOK_CLOCK_HZ}
static const Step *script;
static unsigned script_count,script_index,saves,offs,menus,renders,barriers;
static unsigned settings_reads,dim_calls,restore_calls,sleeps;
static uint32_t clock_ticks;
static SokPowerSettings system_settings;
static SokLoadResult load_result=SOK_LOAD_NEW;
static bool held[256],fail_write,transaction_active,check_snapshot;
static bool opened,writing,exists[2];
static uint8_t records[2][SOK_SAVE_MAX_SIZE];
static uint8_t previous_record[SOK_SAVE_MAX_SIZE];
static size_t previous_length;
static size_t lengths[2],position;
static unsigned active_slot;
static SokState expected_game;
static SokScreen expected_screen;
static SokModal expected_modal;
static jmp_buf finished;
static keydev_t keyboard;
static uint16_t dummy_vram;
uint16_t *gint_vram=&dummy_vram;

static int mock_open(void *context,unsigned slot,bool write)
{
    (void)context;assert(transaction_active && !opened && slot<2);
    if(!write && !exists[slot])return SOK_IO_ABSENT;
    opened=true;writing=write;active_slot=slot;position=0;
    if(write){exists[slot]=true;lengths[slot]=0;}
    return (int)slot;
}
static ptrdiff_t mock_read(void *context,int fd,void *buffer,size_t length)
{
    (void)context;assert(transaction_active && opened && !writing);
    assert(fd==(int)active_slot);
    if(length>lengths[active_slot]-position)length=lengths[active_slot]-position;
    memcpy(buffer,records[active_slot]+position,length);position+=length;
    return (ptrdiff_t)length;
}
static ptrdiff_t mock_write(void *context,int fd,const void *buffer,size_t length)
{
    (void)context;assert(transaction_active && opened && writing);
    assert(fd==(int)active_slot);
    if(fail_write)return -1;
    assert(position+length<=SOK_SAVE_MAX_SIZE);
    memcpy(records[active_slot]+position,buffer,length);position+=length;
    lengths[active_slot]=position;return (ptrdiff_t)length;
}
static int mock_close(void *context,int fd)
{
    (void)context;assert(transaction_active && opened && fd==(int)active_slot);
    opened=false;return 0;
}
static const SokStorageIO io={NULL,mock_open,mock_read,mock_write,mock_close};
SokLoadResult sok_storage_load(SokProgress *progress)
{sok_progress_init(progress);return load_result;}
bool sok_storage_save(SokProgress *progress)
{
    assert(!transaction_active && !opened);++saves;transaction_active=true;
    bool ok=sok_storage_save_io(progress,&io);
    assert(!opened);transaction_active=false;return ok;
}
static bool same_state(const SokState *a,const SokState *b)
{
    return a->player==b->player && a->moves==b->moves && a->pushes==b->pushes
        && a->undo_count==b->undo_count
        && memcmp(a->crates,b->crates,sizeof(a->crates))==0
        && memcmp(a->undo,b->undo,sizeof(a->undo))==0;
}
void gint_poweroff(bool show_logo)
{
    assert(show_logo && !transaction_active && !opened);++offs;
    assert(app.screen==expected_screen && app.modal==expected_modal);
    assert(app.power_save_failed==fail_write);
    if(check_snapshot) {
        assert(same_state(&app.game,&expected_game));
        assert(same_state(&app.progress.levels[0],&expected_game));
        assert(app.progress.dirty==fail_write);
        if(!fail_write) {
            SokProgress restored;
            transaction_active=true;
            assert(sok_storage_load_io(&restored,&io)==SOK_LOAD_OK);
            transaction_active=false;
            if(app.progress.in_progress[0])
                assert(same_state(&restored.levels[0],&expected_game));
            assert(restored.in_progress[0]==app.progress.in_progress[0]);
            assert(restored.cleared[0]==app.progress.cleared[0]);
        }
    }
}
void gint_osmenu(void)
{
    assert(!transaction_active && !opened);++menus;
    /* Simulate changing settings and the clock while in SYSTEM. */
    system_settings=(SokPowerSettings){60,6,3};clock_ticks=123456;
}
uint32_t rtc_ticks(void){return clock_ticks%SOK_DAY_TICKS;}
void sleep(void){++sleeps;}
void sok_system_power_settings(SokPowerSettings *settings)
{*settings=system_settings;++settings_reads;}
void sok_system_backlight(int level)
{
    assert(!transaction_active && !opened);
    if(level<0 || level>5)return;
    if(level==0)++dim_calls;else {assert(level==system_settings.brightness);++restore_calls;}
}
bool keydev_idle(keydev_t *device,...)
{
    assert(device==&keyboard);
    for(unsigned i=0;i<256;i++)if(held[i])return false;
    return true;
}
void dsetvram(uint16_t *first,uint16_t *second)
{assert(first==gint_vram && !second);}
void sok_render(const SokApp *current){assert(current==&app);++renders;}
void clearevents(void){++barriers;}
bool keydown(int key){assert(key>=0 && key<256);return held[key];}
keydev_t *keydev_std(void){return &keyboard;}
void keydev_set_transform(keydev_t *device,keydev_transform_t transform)
{
    assert(device==&keyboard && transform.enabled==KEYDEV_TR_REPEATS);
    assert(transform.repeater(KEY_ACON,0,0)==-1);
    assert(transform.repeater(KEY_SHIFT,0,0)==-1);
    assert(transform.repeater(KEY_RIGHT,0,0)==500000);
    assert(transform.repeater(KEY_RIGHT,0,1)==125000);
}
key_event_t keydev_read(keydev_t *device,bool wait,volatile int *timeout)
{
    assert(device==&keyboard && !wait && !timeout);
    while(script_index<script_count) {
        Step next=script[script_index++];
        if(next.check){next.check();continue;}
        clock_ticks=(clock_ticks+next.elapsed)%SOK_DAY_TICKS;
        if(next.event.type==KEYEV_DOWN)held[next.event.key]=true;
        else if(next.event.type==KEYEV_UP)held[next.event.key]=false;
        return next.event;
    }
    longjmp(finished,1);
}
static void run(const Step *steps,unsigned count)
{
    memset(held,0,sizeof(held));memset(exists,0,sizeof(exists));
    script=steps;script_count=count;script_index=0;
    saves=offs=menus=renders=barriers=0;
    settings_reads=dim_calls=restore_calls=sleeps=0;clock_ticks=0;
    system_settings=(SokPowerSettings){10,1,4};
    fail_write=false;transaction_active=false;opened=false;check_snapshot=false;
    expected_screen=SOK_MAIN;expected_modal=SM_NONE;
    if(setjmp(finished)==0)(void)sok_native_main();
    assert(!transaction_active && !opened && renders>0 && barriers>0);
}
#define RUN(steps_) run(steps_,(unsigned)(sizeof(steps_)/sizeof((steps_)[0])))
static void assert_none(void){assert(offs==0 && saves==0);}
static void assert_once_clean(void){assert(offs==1 && saves==0);}
static void assert_once_saved(void){assert(offs==1 && saves==1);}
static void expect_levels(void){expected_screen=SOK_LEVELS;}
static void expect_play(void){expected_screen=SOK_PLAY;}
static void check_input(void)
{
    const Step steps[]={DOWN(KEY_SHIFT),UP(KEY_SHIFT),CHECK(assert_none),
        DOWN(KEY_SHIFT),UP(KEY_SHIFT),DOWN(KEY_ACON),UP(KEY_ACON),CHECK(assert_none),
        DOWN(KEY_SHIFT),UP(KEY_SHIFT),DOWN(KEY_0),UP(KEY_0),
        DOWN(KEY_ACON),UP(KEY_ACON),CHECK(assert_none),
        DOWN(KEY_SHIFT),UP(KEY_SHIFT),DOWN(KEY_ALPHA),UP(KEY_ALPHA),
        DOWN(KEY_ACON),UP(KEY_ACON),CHECK(assert_none),
        DOWN(KEY_ACON),UP(KEY_ACON),CHECK(assert_none),
        DOWN(KEY_SHIFT),UP(KEY_SHIFT),DOWN(KEY_ACON),CHECK(assert_once_clean),
        HOLD(KEY_ACON),DOWN(KEY_ACON),CHECK(assert_once_clean),UP(KEY_ACON),
        HOLD(KEY_ACON),DOWN(KEY_ACON),UP(KEY_ACON),CHECK(assert_once_clean)};
    RUN(steps);
    const Step held_steps[]={DOWN(KEY_SHIFT),DOWN(KEY_ACON),CHECK(assert_once_clean),
        UP(KEY_ACON),DOWN(KEY_ACON),UP(KEY_ACON),CHECK(assert_once_clean),
        UP(KEY_SHIFT),DOWN(KEY_SHIFT),DOWN(KEY_ACON)};
    RUN(held_steps);assert(offs==2 && saves==0);
    const Step reversed[]={DOWN(KEY_ACON),DOWN(KEY_SHIFT),CHECK(assert_none),
        HOLD(KEY_ACON),CHECK(assert_none),UP(KEY_ACON),UP(KEY_SHIFT),
        DOWN(KEY_ACON),CHECK(assert_once_clean)};
    RUN(reversed);
}
static void check_barriers(void)
{
    const Step steps[]={DOWN(KEY_SHIFT),DOWN(KEY_EXE),CHECK(expect_levels),
        UP(KEY_EXE),DOWN(KEY_ACON),UP(KEY_ACON),CHECK(assert_none),
        UP(KEY_SHIFT),DOWN(KEY_SHIFT),UP(KEY_SHIFT),DOWN(KEY_ACON),
        CHECK(assert_once_clean)};
    RUN(steps);
    const Step level_steps[]={DOWN(KEY_EXE),UP(KEY_EXE),CHECK(expect_levels),
        DOWN(KEY_SHIFT),UP(KEY_SHIFT),DOWN(KEY_ACON),CHECK(assert_once_clean)};
    RUN(level_steps);
    const Step play_steps[]={DOWN(KEY_EXE),UP(KEY_EXE),DOWN(KEY_EXE),UP(KEY_EXE),
        CHECK(expect_play),DOWN(KEY_SHIFT),UP(KEY_SHIFT),DOWN(KEY_ACON),
        CHECK(assert_once_clean)};
    RUN(play_steps);
}
static void dirty_move(void)
{
    assert(sok_app_key(&app,SK_EXE));assert(sok_app_key(&app,SK_EXE));
    const SokMap *map=sok_get_map(1);
    for(unsigned d=0;d<4;d++) {
        SokState trial=app.game;
        if(sok_move(map,&trial,(SokDirection)d)) {
            assert(sok_app_key(&app,(SokKey)d));break;
        }
    }
    assert(app.progress.dirty && app.game.moves>0);
    expected_game=app.game;check_snapshot=true;expected_screen=SOK_PLAY;
}
static void dirty_push(void)
{
    dirty_move();const SokMap *map=sok_get_map(1);
    for(unsigned cell=0;cell<(unsigned)map->width*map->height;cell++) {
        for(unsigned d=0;d<4;d++) {
            SokState trial=app.game;trial.player=(uint16_t)cell;trial.undo_count=0;
            memset(trial.undo,0,sizeof(trial.undo));
            if(!sok_validate(map,&trial))continue;
            SokState moved=trial;
            if(sok_move(map,&moved,(SokDirection)d) && moved.pushes>trial.pushes
                && !sok_solved(map,&moved)) {
                app.game=trial;assert(sok_app_key(&app,(SokKey)d));
                expected_game=app.game;return;
            }
        }
    }
    assert(0 && "fixture needs a legal push");
}
static void dirty_undo(void)
{dirty_push();assert(sok_app_key(&app,SK_F2));expected_game=app.game;}
static void dirty_init_modal(void)
{dirty_push();assert(sok_app_key(&app,SK_F1));expected_modal=SM_INIT;}
static void dirty_win_modal(void)
{
    dirty_move();const SokMap *map=sok_get_map(1);unsigned goal=0;
    for(unsigned cell=0;cell<(unsigned)map->width*map->height;cell++) {
        if(sok_map_terrain(map,(uint16_t)cell)==SOK_GOAL)
            app.game.crates[goal++]=(uint16_t)cell;
        else if(sok_map_terrain(map,(uint16_t)cell)==SOK_FLOOR)
            app.game.player=(uint16_t)cell;
    }
    app.game.moves=1000;app.game.pushes=100;
    app.game.undo_count=0;memset(app.game.undo,0,sizeof(app.game.undo));
    assert(sok_solved(map,&app.game));
    assert(sok_progress_checkpoint(&app.progress,1,&app.game,true));
    app.modal=SM_WIN;expected_modal=SM_WIN;expected_game=app.game;
}
static void dirty_save_error(void)
{
    dirty_move();fail_write=true;
    assert(sok_app_key(&app,SK_EXIT));assert(saves==1 && app.modal==SM_SAVE_ERROR);
    saves=0;expected_modal=SM_SAVE_ERROR;
}
static void dirty_completed_board(void)
{
    dirty_win_modal();assert(sok_app_key(&app,SK_EXIT));
    assert(app.modal==SM_NONE && app.screen==SOK_PLAY);expected_modal=SM_NONE;
}
static void dirty_main(void)
{
    dirty_move();assert(sok_progress_checkpoint(&app.progress,1,&app.game,false));
    app.screen=SOK_MAIN;expected_screen=SOK_MAIN;
}
static void dirty_levels(void)
{dirty_main();app.screen=SOK_LEVELS;expected_screen=SOK_LEVELS;}
static void dirty_load_notice(void)
{dirty_main();sok_app_load_notice(&app,false);expected_modal=SM_LOAD_NOTICE;}
static void fail_save(void){fail_write=true;}
static void previous_save_then_change(void)
{
    dirty_push();assert(sok_progress_checkpoint(&app.progress,1,&app.game,false));
    assert(sok_storage_save(&app.progress));
    previous_length=lengths[0];memcpy(previous_record,records[0],previous_length);
    const SokMap *map=sok_get_map(1);
    for(unsigned d=0;d<4;d++) {
        SokState trial=app.game;
        if(sok_move(map,&trial,(SokDirection)d)) {
            assert(sok_app_key(&app,(SokKey)d));break;
        }
    }
    assert(app.progress.dirty);expected_game=app.game;fail_write=true;saves=0;
}
static void check_dirty_screens(void)
{
    void (*const fixtures[])(void)={dirty_move,dirty_push,dirty_undo,dirty_init_modal,
        dirty_win_modal,dirty_completed_board,dirty_save_error,dirty_main,dirty_levels,dirty_load_notice};
    for(unsigned i=0;i<sizeof(fixtures)/sizeof(fixtures[0]);i++) {
        const Step steps[]={CHECK(fixtures[i]),DOWN(KEY_SHIFT),UP(KEY_SHIFT),
            DOWN(KEY_ACON),CHECK(assert_once_saved),HOLD(KEY_ACON),DOWN(KEY_ACON),
            CHECK(assert_once_saved)};
        RUN(steps);
        assert(app.power_save_failed==fail_write);
    }
    const Step failed[]={CHECK(dirty_push),CHECK(fail_save),DOWN(KEY_SHIFT),
        UP(KEY_SHIFT),DOWN(KEY_ACON),CHECK(assert_once_saved)};
    RUN(failed);assert(app.progress.dirty && app.progress.generation==0);
    const Step previous[]={CHECK(previous_save_then_change),DOWN(KEY_SHIFT),
        UP(KEY_SHIFT),DOWN(KEY_ACON),CHECK(assert_once_saved)};
    RUN(previous);assert(app.progress.dirty && app.progress.generation==1);
    assert(lengths[0]==previous_length && memcmp(records[0],previous_record,previous_length)==0);
    const Step twice[]={CHECK(dirty_push),DOWN(KEY_SHIFT),UP(KEY_SHIFT),
        DOWN(KEY_ACON),CHECK(assert_once_saved),UP(KEY_ACON),DOWN(KEY_SHIFT),
        UP(KEY_SHIFT),DOWN(KEY_ACON)};
    RUN(twice);assert(offs==2 && saves==1 && !app.progress.dirty);
}
static void assert_dimmed(void){assert(dim_calls==1 && restore_calls==0 && offs==0);}
static void assert_restored(void){assert(dim_calls==1 && restore_calls==1 && offs==0);}
static void assert_no_dim(void){assert(dim_calls==0 && offs==0);}
static void assert_auto_saved(void)
{
    assert_once_saved();assert(settings_reads==2);
    assert(!idle.dimmed && idle.last_activity==rtc_ticks());
    if(fail_write)assert(app.modal==SM_SAVE_ERROR && app.progress.dirty);
}
static void expect_startup_notice(void)
{
    expected_modal=SM_LOAD_NOTICE;
    assert(app.modal==SM_LOAD_NOTICE);
    assert(app.recovered_notice==(load_result==SOK_LOAD_RECOVERED));
}
static void invalid_brightness(void){power_settings.brightness=0;}
static void check_idle_lifecycle(void)
{
    const Step dim[]={WAIT(29),CHECK(assert_no_dim),WAIT(1),CHECK(assert_dimmed),
        WAIT(100),CHECK(assert_dimmed),DOWN(KEY_0),CHECK(assert_restored),UP(KEY_0),
        WAIT(29),CHECK(assert_restored),WAIT(1)};
    RUN(dim);assert(dim_calls==2 && restore_calls==1 && sleeps>0);
    const Step held_key[]={DOWN(KEY_0),WAIT(3601),CHECK(assert_no_dim),
        UP(KEY_0),WAIT(29),CHECK(assert_no_dim),WAIT(1),CHECK(assert_dimmed)};
    RUN(held_key);
    void (*const fixtures[])(void)={dirty_move,dirty_push,dirty_undo,dirty_init_modal,
        dirty_win_modal,dirty_completed_board,dirty_save_error,dirty_main,dirty_levels,dirty_load_notice};
    for(unsigned i=0;i<sizeof(fixtures)/sizeof(fixtures[0]);i++) {
        const Step steps[]={CHECK(fixtures[i]),WAIT(30),WAIT(569),CHECK(assert_none),
            WAIT(1),CHECK(assert_auto_saved),WAIT(1)};
        RUN(steps);assert(offs==1 && saves==1 && dim_calls==1 && restore_calls==1);
    }
    const Step failed[]={CHECK(previous_save_then_change),WAIT(600),CHECK(assert_auto_saved)};
    RUN(failed);assert(app.progress.dirty && app.progress.generation==1);
    assert(lengths[0]==previous_length && memcmp(records[0],previous_record,previous_length)==0);
    const Step reload[]={WAIT(30),DOWN(KEY_MENU),UP(KEY_MENU),WAIT(179),WAIT(1),WAIT(3420)};
    RUN(reload);assert(menus==1 && offs==1 && settings_reads==3 && dim_calls==2 && restore_calls==2);
    const Step clean[]={WAIT(600),CHECK(assert_once_clean),WAIT(599),CHECK(assert_once_clean),WAIT(1)};
    RUN(clean);assert(offs==2 && saves==0);
    const Step invalid[]={CHECK(invalid_brightness),WAIT(30),DOWN(KEY_0),UP(KEY_0),
        WAIT(600),CHECK(assert_once_clean)};
    RUN(invalid);assert(!dim_calls && !restore_calls);
    const SokLoadResult notices[]={SOK_LOAD_INVALID,SOK_LOAD_IO_ERROR,SOK_LOAD_RECOVERED};
    for(unsigned i=0;i<sizeof(notices)/sizeof(notices[0]);i++) {
        load_result=notices[i];
        const Step notice[]={CHECK(expect_startup_notice),WAIT(600),CHECK(assert_once_clean),
            DOWN(KEY_EXE),UP(KEY_EXE)};
        RUN(notice);assert(app.modal==SM_NONE);
    }
    load_result=SOK_LOAD_NEW;
}
int main(void)
{
    assert(KEY_SHIFT==0x81 && KEY_ACON==0x07 && native_keys[SK_SHIFT]==KEY_SHIFT
        && native_keys[SK_ACON]==KEY_ACON);
    check_input();check_barriers();check_dirty_screens();check_idle_lifecycle();
    puts("power: manual/automatic OFF, all screens, idle dim/wake/held keys, SYSTEM reload and save failures passed");
    return 0;
}

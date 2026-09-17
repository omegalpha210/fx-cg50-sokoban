#include "app.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned assertions;
#define CHECK(expression) do { \
    ++assertions; \
    if (!(expression)) { \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expression); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

typedef struct FakeHooks {
    unsigned saves;
    unsigned os_calls;
    unsigned saves_at_os;
    bool fail;
    bool dirty_at_os;
    SokApp *app;
    SokProgress persisted;
} FakeHooks;

static bool fake_save(void *context, SokProgress *progress)
{
    FakeHooks *fake = context;
    ++fake->saves;
    if (fake->fail) return false;
    ++progress->generation;
    progress->dirty = false;
    fake->persisted = *progress;
    return true;
}

static void fake_os(void *context)
{
    FakeHooks *fake = context;
    ++fake->os_calls;
    fake->saves_at_os = fake->saves;
    fake->dirty_at_os = fake->app->progress.dirty;
}

static void init(SokApp *app, FakeHooks *fake)
{
    memset(fake, 0, sizeof(*fake));
    fake->app = app;
    SokHooks hooks = {.save=fake_save, .os_menu=fake_os, .context=fake};
    sok_app_init(app, hooks);
}

static void open_level(SokApp *app, unsigned level)
{
    app->screen = SOK_LEVELS;
    app->modal = SM_NONE;
    app->group = (level - 1u) / 15u;
    app->selection = (level - 1u) % 15u;
    CHECK(sok_app_key(app, SK_EXE));
    CHECK(app->screen == SOK_PLAY && app->level == level);
    CHECK(sok_validate(sok_get_map(level), &app->game));
}

static bool board_equal(const SokMap *map, const SokState *a, const SokState *b)
{
    return a->player == b->player && a->moves == b->moves &&
        a->pushes == b->pushes && a->undo_count == b->undo_count &&
        memcmp(a->crates, b->crates, map->crate_count * sizeof(a->crates[0])) == 0 &&
        memcmp(a->undo, b->undo, sizeof(a->undo)) == 0;
}

static SokKey first_move(SokApp *app)
{
    const SokMap *map = sok_get_map(app->level);
    for (unsigned direction = 0; direction < 4; ++direction) {
        SokState trial = app->game;
        if (sok_move(map, &trial, (SokDirection)direction)) {
            CHECK(sok_app_key(app, (SokKey)direction));
            return (SokKey)direction;
        }
    }
    CHECK(false);
    return SK_NONE;
}

static void test_main_and_level_grid(void)
{
    SokApp app;
    FakeHooks fake;
    init(&app, &fake);
    CHECK(app.screen == SOK_MAIN && app.group == 0 && app.modal == SM_NONE);
    CHECK(!sok_app_key(&app, SK_EXIT));
    CHECK(app.screen == SOK_MAIN);
    for (unsigned i = 1; i <= 4; ++i) {
        CHECK(sok_app_key(&app, SK_RIGHT)); CHECK(app.group == i % 4u);
    }
    for (unsigned i = 1; i <= 4; ++i) {
        CHECK(sok_app_key(&app, SK_LEFT)); CHECK(app.group == (4u - i) % 4u);
    }
    for (unsigned column = 0; column < 2; ++column) {
        app.group = column;
        CHECK(sok_app_key(&app, SK_UP)); CHECK(app.group == column + 2u);
        CHECK(sok_app_key(&app, SK_DOWN)); CHECK(app.group == column);
        CHECK(sok_app_key(&app, SK_DOWN)); CHECK(app.group == column + 2u);
        CHECK(sok_app_key(&app, SK_UP)); CHECK(app.group == column);
    }
    for (unsigned group = 0; group < 4; ++group) {
        CHECK(sok_app_key(&app, (SokKey)(SK_1 + group)));
        CHECK(app.screen == SOK_LEVELS && app.group == group && app.selection == 0);
        /* Traverse every boundary in both directions in every group. */
        for (unsigned i = 1; i <= 15; ++i) {
            CHECK(sok_app_key(&app, SK_RIGHT)); CHECK(app.selection == i % 15u);
        }
        for (unsigned i = 1; i <= 15; ++i) {
            CHECK(sok_app_key(&app, SK_LEFT)); CHECK(app.selection == (15u - i) % 15u);
        }
        for (unsigned column = 0; column < 5; ++column) {
            app.selection = column;
            CHECK(sok_app_key(&app, SK_UP)); CHECK(app.selection == column + 10u);
            CHECK(sok_app_key(&app, SK_DOWN)); CHECK(app.selection == column);
            CHECK(sok_app_key(&app, SK_DOWN)); CHECK(app.selection == column + 5u);
            CHECK(sok_app_key(&app, SK_DOWN)); CHECK(app.selection == column + 10u);
            CHECK(sok_app_key(&app, SK_DOWN)); CHECK(app.selection == column);
        }
        app.selection = 4;
        CHECK(sok_app_key(&app, SK_RIGHT)); CHECK(app.selection == 5);
        CHECK(sok_app_key(&app, SK_F6));
        CHECK(app.level == group * 15u + 6u && app.screen == SOK_PLAY);
        CHECK(sok_app_key(&app, SK_EXIT));
        CHECK(app.screen == SOK_LEVELS && app.selection == 5 && app.group == group);
        CHECK(sok_app_key(&app, SK_EXIT));
        CHECK(app.screen == SOK_MAIN && app.group == group);
    }
    CHECK(sok_app_key(&app, SK_F6));
    CHECK(app.screen == SOK_LEVELS && app.group == 3);
    CHECK(sok_app_key(&app, SK_EXIT));
    CHECK(sok_app_key(&app, SK_EXE));
    CHECK(app.screen == SOK_LEVELS);
    CHECK(sok_app_key(&app, SK_EXE));
    CHECK(app.screen == SOK_PLAY && app.level == 46);
}

static void test_boundaries_and_checkpoint_resume(void)
{
    SokApp app;
    FakeHooks fake;
    init(&app, &fake);
    const unsigned boundaries[] = {15, 30, 45};
    for (unsigned i = 0; i < 3; ++i) {
        unsigned level = boundaries[i];
        open_level(&app, level);
        SokState original = app.game;
        (void)first_move(&app);
        SokState moved = app.game;
        unsigned saves = fake.saves;
        CHECK(sok_app_key(&app, SK_F6));
        CHECK(app.level == level + 1u && app.group == i + 1u);
        CHECK(fake.saves == saves + 1u && !app.progress.dirty);
        CHECK(app.progress.in_progress[level - 1u]);
        CHECK(board_equal(sok_get_map(level), &app.progress.levels[level - 1u], &moved));
        CHECK(sok_app_key(&app, SK_F5));
        CHECK(app.level == level && board_equal(sok_get_map(level), &app.game, &moved));
        CHECK(sok_app_key(&app, SK_F2));
        CHECK(board_equal(sok_get_map(level), &app.game, &original));
    }
    open_level(&app, 1);
    unsigned saves = fake.saves;
    SokState initial = app.game;
    CHECK(!sok_app_key(&app, SK_F5));
    CHECK(app.level == 1 && fake.saves == saves);
    CHECK(board_equal(sok_get_map(1), &app.game, &initial));
    open_level(&app, 60);
    saves = fake.saves;
    initial = app.game;
    CHECK(!sok_app_key(&app, SK_F6));
    CHECK(app.level == 60 && fake.saves == saves);
    CHECK(board_equal(sok_get_map(60), &app.game, &initial));
}

static void test_init_and_independent_progress(void)
{
    SokApp app;
    FakeHooks fake;
    init(&app, &fake);
    open_level(&app, 2);
    (void)first_move(&app);
    SokState other = app.game;
    CHECK(sok_app_key(&app, SK_F5));
    CHECK(app.level == 1);
    app.progress.cleared[0] = 1;
    app.progress.cleared[1] = 1;
    (void)first_move(&app);
    SokState current = app.game;
    unsigned saves = fake.saves;
    CHECK(sok_app_key(&app, SK_F1));
    CHECK(app.modal == SM_INIT);
    CHECK(board_equal(sok_get_map(1), &app.game, &current));
    CHECK(!sok_app_key(&app, SK_RIGHT));
    CHECK(sok_app_key(&app, SK_EXIT));
    CHECK(app.modal == SM_NONE && fake.saves == saves);
    CHECK(board_equal(sok_get_map(1), &app.game, &current));
    CHECK(sok_app_key(&app, SK_F1));
    CHECK(sok_app_key(&app, SK_EXE));
    CHECK(app.modal == SM_NONE && app.screen == SOK_PLAY);
    CHECK(app.game.moves == 0 && app.game.pushes == 0 && app.game.undo_count == 0);
    CHECK(app.progress.cleared[0] == 1 && app.progress.cleared[1] == 1);
    CHECK(fake.saves == saves + 1u && !app.progress.dirty);
    CHECK(board_equal(sok_get_map(2), &app.progress.levels[1], &other));
    SokState reset = app.game;
    CHECK(sok_app_key(&app, SK_EXIT));
    CHECK(sok_app_key(&app, SK_EXE));
    CHECK(board_equal(sok_get_map(1), &app.game, &reset));
    CHECK(sok_app_key(&app, SK_F6));
    CHECK(board_equal(sok_get_map(2), &app.game, &other));
    CHECK(sok_app_key(&app, SK_F2));
    CHECK(app.game.moves == 0 && app.progress.cleared[1] == 1);
}

/* Synthesize a validated dynamic snapshot one push from completion. This
 * tests lifecycle behavior; it does not claim to solve an upstream puzzle. */
static SokKey near_completion(SokApp *app)
{
    const SokMap *map = sok_get_map(app->level);
    SokState state = {0};
    unsigned goals = 0;
    for (unsigned i = 0; i < (unsigned)map->width * map->height; ++i)
        if (sok_map_terrain(map, (uint16_t)i) == SOK_GOAL)
            state.crates[goals++] = (uint16_t)i;
    CHECK(goals == map->crate_count);
    static const int dx[] = {0, 1, 0, -1};
    static const int dy[] = {-1, 0, 1, 0};
    for (unsigned goal = 0; goal < goals; ++goal) {
        int gx = state.crates[goal] % map->width;
        int gy = state.crates[goal] / map->width;
        for (unsigned direction = 0; direction < 4; ++direction) {
            int bx = gx - dx[direction], by = gy - dy[direction];
            int px = gx - 2 * dx[direction], py = gy - 2 * dy[direction];
            if (bx < 0 || by < 0 || px < 0 || py < 0 || bx >= map->width ||
                by >= map->height || px >= map->width || py >= map->height) continue;
            uint16_t box = (uint16_t)(by * map->width + bx);
            uint16_t player = (uint16_t)(py * map->width + px);
            if (sok_map_terrain(map, box) != SOK_FLOOR ||
                sok_map_terrain(map, player) != SOK_FLOOR) continue;
            state.crates[goal] = box;
            state.player = player;
            state.moves = 1000;
            state.pushes = 100;
            CHECK(sok_validate(map, &state));
            CHECK(!sok_solved(map, &state));
            app->game = state;
            return (SokKey)direction;
        }
    }
    CHECK(false);
    return SK_NONE;
}

static void test_completion_and_retry(void)
{
    SokApp app;
    FakeHooks fake;
    init(&app, &fake);
    const unsigned levels[] = {1, 15, 30, 45, 60};
    for (unsigned i = 0; i < 5; ++i) {
        unsigned level = levels[i];
        open_level(&app, level);
        SokKey last = near_completion(&app);
        unsigned saves = fake.saves;
        CHECK(sok_app_key(&app, last));
        CHECK(app.modal == SM_WIN && app.progress.cleared[level - 1u] == 1);
        CHECK(app.progress.in_progress[level - 1u] == 0);
        CHECK(fake.saves == saves + 1u && !app.progress.dirty);
        SokState complete = app.game;
        CHECK(!sok_app_key(&app, last));
        CHECK(!sok_app_key(&app, SK_F2));
        CHECK(!sok_app_key(&app, SK_F1));
        CHECK(fake.saves == saves + 1u);
        CHECK(board_equal(sok_get_map(level), &app.game, &complete));
        CHECK(sok_app_key(&app, SK_EXE));
        if (level == 60) {
            CHECK(app.screen == SOK_LEVELS && app.group == 3 && app.selection == 14);
            CHECK(app.level == 60);
        } else {
            CHECK(app.screen == SOK_PLAY && app.level == level + 1u);
        }
        open_level(&app, level);
        CHECK(app.game.moves == 0 && app.game.undo_count == 0 && app.modal == SM_NONE);
        CHECK(app.progress.cleared[level - 1u] == 1);
        (void)first_move(&app);
        SokState retry = app.game;
        CHECK(sok_app_key(&app, SK_EXIT));
        CHECK(sok_app_key(&app, SK_EXE));
        CHECK(board_equal(sok_get_map(level), &app.game, &retry));
        CHECK(app.modal == SM_NONE && app.progress.cleared[level - 1u] == 1);
        last = near_completion(&app);
        CHECK(sok_app_key(&app, last));
        CHECK(sok_app_key(&app, SK_EXIT));
        CHECK(app.screen == SOK_LEVELS && app.selection == (level - 1u) % 15u);
    }
}

static void test_save_failure_paths(void)
{
    SokApp app;
    FakeHooks fake;
    init(&app, &fake);
    open_level(&app, 1);
    (void)first_move(&app);
    SokState before = app.game;
    fake.fail = true;
    CHECK(sok_app_key(&app, SK_EXIT));
    CHECK(app.modal == SM_SAVE_ERROR && app.screen == SOK_PLAY && app.progress.dirty);
    CHECK(fake.saves == 1 && app.progress.generation == 0);
    CHECK(board_equal(sok_get_map(1), &app.game, &before));
    CHECK(!sok_app_key(&app, SK_RIGHT));
    CHECK(sok_app_key(&app, SK_EXE));
    CHECK(fake.saves == 2 && app.modal == SM_SAVE_ERROR && app.progress.dirty);
    CHECK(sok_app_key(&app, SK_EXIT));
    CHECK(app.screen == SOK_PLAY && app.modal == SM_NONE && app.progress.dirty);
    CHECK(board_equal(sok_get_map(1), &app.game, &before));
    CHECK(sok_app_key(&app, SK_EXIT));
    CHECK(app.modal == SM_SAVE_ERROR);
    CHECK(sok_app_key(&app, SK_F6));
    CHECK(app.screen == SOK_LEVELS && app.modal == SM_NONE && app.progress.dirty);
    CHECK(app.progress.generation == 0);
    CHECK(sok_app_key(&app, SK_EXE));
    CHECK(board_equal(sok_get_map(1), &app.game, &before));
    CHECK(sok_app_key(&app, SK_EXIT));
    fake.fail = false;
    CHECK(sok_app_key(&app, SK_EXE));
    CHECK(app.screen == SOK_LEVELS && !app.progress.dirty);
    CHECK(app.progress.generation == 1);
    CHECK(board_equal(sok_get_map(1), &fake.persisted.levels[0], &before));

    open_level(&app, 15);
    SokKey last = near_completion(&app);
    fake.fail = true;
    CHECK(sok_app_key(&app, last));
    CHECK(app.modal == SM_SAVE_ERROR && app.progress.cleared[14] == 1);
    CHECK(sok_app_key(&app, SK_EXIT));
    CHECK(app.modal == SM_WIN && app.progress.dirty);
    CHECK(sok_app_key(&app, SK_EXE));
    CHECK(app.modal == SM_SAVE_ERROR && app.level == 15);
    CHECK(sok_app_key(&app, SK_F6));
    CHECK(app.level == 16 && app.modal == SM_NONE && app.progress.dirty);
    CHECK(app.progress.cleared[14] == 1);
}

static void test_menu_lifecycle_and_load_notice(void)
{
    SokApp app;
    FakeHooks fake;
    init(&app, &fake);
    CHECK(sok_app_key(&app, SK_MENU));
    CHECK(fake.os_calls == 1 && fake.saves == 0 && app.screen == SOK_MAIN);
    CHECK(sok_app_key(&app, SK_2));
    CHECK(sok_app_key(&app, SK_MENU));
    CHECK(fake.os_calls == 2 && fake.saves == 0 && app.screen == SOK_LEVELS);
    CHECK(sok_app_key(&app, SK_EXE));
    (void)first_move(&app);
    SokState before = app.game;
    unsigned level = app.level;
    CHECK(sok_app_key(&app, SK_MENU));
    CHECK(fake.os_calls == 3 && fake.saves_at_os == 1 && !fake.dirty_at_os);
    CHECK(app.screen == SOK_PLAY && app.level == level && app.modal == SM_NONE);
    CHECK(board_equal(sok_get_map(level), &app.game, &before));
    CHECK(sok_app_key(&app, SK_F1));
    CHECK(sok_app_key(&app, SK_MENU));
    CHECK(app.modal == SM_INIT && fake.os_calls == 4);
    CHECK(sok_app_key(&app, SK_EXIT));
    (void)first_move(&app);
    fake.fail = true;
    CHECK(sok_app_key(&app, SK_MENU));
    CHECK(fake.os_calls == 4 && app.modal == SM_SAVE_ERROR);
    CHECK(sok_app_key(&app, SK_F6));
    CHECK(fake.os_calls == 5 && fake.dirty_at_os && app.progress.dirty);
    CHECK(app.screen == SOK_PLAY && app.modal == SM_NONE);

    SokApp restarted;
    FakeHooks restart_fake;
    init(&restarted, &restart_fake);
    restarted.progress = fake.persisted;
    CHECK(restarted.screen == SOK_MAIN);
    open_level(&restarted, level);
    CHECK(board_equal(sok_get_map(level), &restarted.game, &before));
    sok_app_load_notice(&restarted, true);
    CHECK(restarted.modal == SM_LOAD_NOTICE && restarted.recovered_notice);
    CHECK(!sok_app_key(&restarted, SK_RIGHT));
    CHECK(sok_app_key(&restarted, SK_EXE));
    CHECK(restarted.modal == SM_NONE);
    sok_app_load_notice(&restarted, false);
    CHECK(!restarted.recovered_notice);
    CHECK(sok_app_key(&restarted, SK_EXIT));
    CHECK(restarted.modal == SM_NONE);
}

static void release(SokApp *app, SokKey key)
{
    CHECK(!sok_app_event(app, key, SE_UP));
}

static void test_hold_and_input_barriers(void)
{
    SokApp app;
    FakeHooks fake;
    init(&app, &fake);
    CHECK(sok_app_event(&app, SK_EXE, SE_DOWN));
    CHECK(app.screen == SOK_LEVELS);
    CHECK(!sok_app_event(&app, SK_EXE, SE_HOLD));
    CHECK(!sok_app_event(&app, SK_EXE, SE_DOWN));
    CHECK(app.screen == SOK_LEVELS);
    release(&app, SK_EXE);
    CHECK(sok_app_event(&app, SK_EXE, SE_DOWN));
    CHECK(app.screen == SOK_PLAY);
    release(&app, SK_EXE);
    (void)first_move(&app);
    (void)first_move(&app);
    CHECK(sok_app_event(&app, SK_F2, SE_DOWN));
    CHECK(app.game.moves == 1);
    CHECK(!sok_app_event(&app, SK_F2, SE_HOLD));
    CHECK(!sok_app_event(&app, SK_F2, SE_DOWN));
    CHECK(app.game.moves == 1);
    release(&app, SK_F2);
    CHECK(sok_app_event(&app, SK_F2, SE_DOWN));
    CHECK(app.game.moves == 0);
    release(&app, SK_F2);
    SokKey last = near_completion(&app);
    CHECK(sok_app_event(&app, last, SE_DOWN));
    CHECK(app.modal == SM_WIN);
    CHECK(!sok_app_event(&app, last, SE_HOLD));
    CHECK(sok_app_event(&app, SK_EXE, SE_DOWN));
    CHECK(app.level == 2 && app.game.moves == 0);
    CHECK(!sok_app_event(&app, last, SE_HOLD));
    CHECK(!sok_app_event(&app, last, SE_DOWN));
    CHECK(!sok_app_event(&app, SK_EXE, SE_HOLD));
    CHECK(app.level == 2 && app.game.moves == 0);
    release(&app, last);
    release(&app, SK_EXE);

    SokInput input;
    sok_input_init(&input);
    CHECK(sok_input_event(&input, SK_RIGHT, SE_DOWN));
    CHECK(sok_input_event(&input, SK_RIGHT, SE_HOLD));
    CHECK(!sok_input_event(&input, SK_LEFT, SE_DOWN));
    CHECK(!sok_input_event(&input, SK_LEFT, SE_HOLD));
    CHECK(sok_input_event(&input, SK_RIGHT, SE_HOLD));
    CHECK(!sok_input_event(&input, SK_RIGHT, SE_UP));
    CHECK(!sok_input_event(&input, SK_LEFT, SE_HOLD));
    CHECK(!sok_input_event(&input, SK_LEFT, SE_UP));
    CHECK(sok_input_event(&input, SK_LEFT, SE_DOWN));
    sok_input_barrier(&input);
    CHECK(!sok_input_event(&input, SK_LEFT, SE_HOLD));
    CHECK(!sok_input_event(&input, SK_LEFT, SE_UP));
    CHECK(sok_input_event(&input, SK_LEFT, SE_DOWN));
    CHECK(!sok_input_event(&input, SK_NONE, SE_DOWN));
    CHECK(!sok_input_event(&input, SK_COUNT, SE_DOWN));
    CHECK(!sok_input_event(&input, SK_MENU, SE_HOLD));
    CHECK(sok_input_event(&input, SK_MENU, SE_DOWN));
    CHECK(!sok_input_event(&input, SK_LEFT, SE_UP));
    CHECK(!sok_input_event(&input, SK_LEFT, SE_HOLD));
    CHECK((input.held & (UINT32_C(1) << SK_LEFT)) == 0);
    CHECK(sok_input_event(&input, SK_LEFT, SE_DOWN));
}

int main(void)
{
    test_main_and_level_grid();
    test_boundaries_and_checkpoint_resume();
    test_init_and_independent_progress();
    test_completion_and_retry();
    test_save_failure_paths();
    test_menu_lifecycle_and_load_notice();
    test_hold_and_input_barriers();
    printf("workflow: %u assertions passed (navigation, progress, lifecycle, input)\n",
        assertions);
    return EXIT_SUCCESS;
}

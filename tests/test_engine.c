#include "game.h"

#include <limits.h>
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

typedef struct Fixture {
    SokMap map;
    uint8_t terrain[(SOK_MAX_CELLS + 3u) / 4u];
    uint16_t crates[SOK_MAX_CRATES];
} Fixture;

/* These original test fixtures deliberately use familiar #/$/+ notation.
 * Upstream map notation is handled separately by the map importer. */
static void fixture(Fixture *f, unsigned width, unsigned height, const char *text)
{
    memset(f, 0, sizeof(*f));
    CHECK(width * height == strlen(text));
    CHECK(width * height <= SOK_MAX_CELLS);
    f->map.width = (uint8_t)width;
    f->map.height = (uint8_t)height;
    f->map.terrain = f->terrain;
    f->map.crates = f->crates;
    unsigned players = 0;
    for (unsigned i = 0; i < width * height; ++i) {
        uint8_t terrain = SOK_FLOOR;
        switch (text[i]) {
        case '#': terrain = SOK_WALL; break;
        case '_': terrain = SOK_VOID; break;
        case '.': case '*': case '+': terrain = SOK_GOAL; break;
        default: break;
        }
        f->terrain[i / 4u] |= (uint8_t)(terrain << (2u * (i % 4u)));
        if (text[i] == '@' || text[i] == '+') {
            f->map.player = (uint16_t)i;
            ++players;
        }
        if (text[i] == '$' || text[i] == '*') {
            CHECK(f->map.crate_count < SOK_MAX_CRATES);
            f->crates[f->map.crate_count++] = (uint16_t)i;
        }
    }
    CHECK(players == 1);
}

static bool board_equal(const SokMap *map, const SokState *a, const SokState *b)
{
    return a->player == b->player && a->moves == b->moves &&
        a->pushes == b->pushes &&
        memcmp(a->crates, b->crates, map->crate_count * sizeof(a->crates[0])) == 0;
}

static bool state_equal(const SokMap *map, const SokState *a, const SokState *b)
{
    return board_equal(map, a, b) && a->undo_count == b->undo_count &&
        memcmp(a->undo, b->undo, sizeof(a->undo)) == 0;
}

static void test_walk_and_completion(void)
{
    Fixture f;
    fixture(&f, 7, 5,
        "#######"
        "#  .  #"
        "#  $  #"
        "# @   #"
        "#######");
    SokState s;
    CHECK(sok_init(&f.map, &s));
    SokState initial = s;
    CHECK(sok_validate(&f.map, &s));
    CHECK(!sok_solved(&f.map, &s));
    CHECK(sok_on_goals(&f.map, &s) == 0);
    CHECK(!sok_move(&f.map, &s, SOK_DOWN));
    CHECK(state_equal(&f.map, &s, &initial));
    CHECK(sok_move(&f.map, &s, SOK_RIGHT));
    CHECK(s.moves == 1 && s.pushes == 0 && s.undo_count == 1);
    CHECK(sok_undo(&f.map, &s));
    CHECK(state_equal(&f.map, &s, &initial));
    CHECK(!sok_undo(&f.map, &s));
    /* Complete this original puzzle with the actual solution RIGHT, UP. */
    CHECK(sok_move(&f.map, &s, SOK_RIGHT));
    CHECK(!sok_solved(&f.map, &s));
    CHECK(sok_move(&f.map, &s, SOK_UP));
    CHECK(s.moves == 2 && s.pushes == 1 && sok_on_goals(&f.map, &s) == 1);
    CHECK(sok_solved(&f.map, &s));
    CHECK(sok_validate(&f.map, &s));
    CHECK(sok_undo(&f.map, &s));
    CHECK(s.moves == 1 && s.pushes == 0 && sok_on_goals(&f.map, &s) == 0);
    CHECK(!sok_solved(&f.map, &s));
    CHECK(sok_undo(&f.map, &s));
    CHECK(state_equal(&f.map, &s, &initial));
}

static void test_push_and_goal_preservation(void)
{
    Fixture f;
    fixture(&f, 7, 3, "#######" "#@$.  #" "#######");
    SokState s;
    CHECK(sok_init(&f.map, &s));
    SokState initial = s;
    CHECK(sok_move(&f.map, &s, SOK_RIGHT));
    SokState on_goal = s;
    CHECK(sok_on_goals(&f.map, &s) == 1);
    CHECK(sok_move(&f.map, &s, SOK_RIGHT));
    CHECK(sok_map_terrain(&f.map, s.player) == SOK_GOAL);
    CHECK(sok_on_goals(&f.map, &s) == 0);
    CHECK(s.moves == 2 && s.pushes == 2);
    CHECK(sok_undo(&f.map, &s));
    CHECK(state_equal(&f.map, &s, &on_goal));
    CHECK(sok_undo(&f.map, &s));
    CHECK(state_equal(&f.map, &s, &initial));
    CHECK(sok_move(&f.map, &s, SOK_RIGHT));
    CHECK(sok_move(&f.map, &s, SOK_RIGHT));
    CHECK(sok_move(&f.map, &s, SOK_RIGHT));
    SokState at_wall = s;
    CHECK(!sok_move(&f.map, &s, SOK_RIGHT));
    CHECK(state_equal(&f.map, &s, &at_wall));
    CHECK(sok_move(&f.map, &s, SOK_LEFT));
    CHECK(s.crates[0] == at_wall.crates[0]); /* No pulling. */
    CHECK(sok_map_terrain(&f.map, 10) == SOK_GOAL);

    fixture(&f, 8, 4, "########" "#@.  $ #" "#      #" "########");
    CHECK(sok_init(&f.map, &s));
    CHECK(sok_move(&f.map, &s, SOK_RIGHT));
    CHECK(sok_map_terrain(&f.map, s.player) == SOK_GOAL);
    CHECK(sok_move(&f.map, &s, SOK_RIGHT));
    CHECK(sok_map_terrain(&f.map, 10) == SOK_GOAL);
    CHECK(s.moves == 2 && s.pushes == 0);

    fixture(&f, 6, 3, "######" "#@*  #" "######");
    CHECK(sok_init(&f.map, &s));
    CHECK(sok_on_goals(&f.map, &s) == 1);
    CHECK(sok_move(&f.map, &s, SOK_RIGHT));
    CHECK(sok_on_goals(&f.map, &s) == 0);
    CHECK(sok_undo(&f.map, &s));
    CHECK(sok_on_goals(&f.map, &s) == 1);
}

static void test_blocked_and_boundaries(void)
{
    Fixture f;
    SokState s;
    fixture(&f, 7, 3, "#######" "#@$$..#" "#######");
    CHECK(sok_init(&f.map, &s));
    SokState before = s;
    CHECK(!sok_move(&f.map, &s, SOK_RIGHT));
    CHECK(state_equal(&f.map, &s, &before));
    fixture(&f, 5, 3, "#####" "#@$_." "#####");
    CHECK(sok_init(&f.map, &s));
    before = s;
    CHECK(!sok_move(&f.map, &s, SOK_RIGHT));
    CHECK(state_equal(&f.map, &s, &before));
    fixture(&f, 5, 3, "_@  ." " $   " "     ");
    CHECK(sok_init(&f.map, &s));
    before = s;
    CHECK(!sok_move(&f.map, &s, SOK_LEFT)); /* VOID */
    CHECK(!sok_move(&f.map, &s, SOK_UP)); /* Top edge */
    CHECK(!sok_move(&f.map, &s, (SokDirection)99));
    CHECK(state_equal(&f.map, &s, &before));
    CHECK(sok_move(&f.map, &s, SOK_RIGHT));
    CHECK(sok_move(&f.map, &s, SOK_RIGHT));
    CHECK(sok_move(&f.map, &s, SOK_RIGHT));
    before = s;
    CHECK(!sok_move(&f.map, &s, SOK_RIGHT)); /* No row wrap. */
    CHECK(state_equal(&f.map, &s, &before));
    CHECK(sok_move(&f.map, &s, SOK_DOWN));
    CHECK(sok_move(&f.map, &s, SOK_DOWN));
    before = s;
    CHECK(!sok_move(&f.map, &s, SOK_DOWN));
    CHECK(state_equal(&f.map, &s, &before));

    fixture(&f, 3, 2, " @$" " . ");
    CHECK(sok_init(&f.map, &s));
    before = s;
    CHECK(!sok_move(&f.map, &s, SOK_RIGHT)); /* Crate beyond board. */
    CHECK(state_equal(&f.map, &s, &before));
    fixture(&f, 3, 2, "@ ." " $ ");
    CHECK(sok_init(&f.map, &s));
    before = s;
    CHECK(!sok_move(&f.map, &s, SOK_LEFT));
    CHECK(state_equal(&f.map, &s, &before));
}

static void test_history_limit_and_branch(void)
{
    Fixture f;
    fixture(&f, 7, 7,
        "#######" "#@    #" "#     #" "#     #"
        "# $ . #" "#     #" "#######");
    SokState s;
    CHECK(sok_init(&f.map, &s));
    CHECK(sok_move(&f.map, &s, SOK_RIGHT));
    CHECK(sok_move(&f.map, &s, SOK_RIGHT));
    SokState after_two = s;
    const SokDirection five[] = {SOK_DOWN,SOK_DOWN,SOK_LEFT,SOK_LEFT,SOK_UP};
    for (unsigned i = 0; i < 5; ++i) CHECK(sok_move(&f.map, &s, five[i]));
    CHECK(s.moves == 7 && s.undo_count == 5);
    CHECK(sok_validate(&f.map, &s));
    for (unsigned i = 0; i < 5; ++i) CHECK(sok_undo(&f.map, &s));
    CHECK(board_equal(&f.map, &s, &after_two));
    CHECK(s.undo_count == 0 && !sok_undo(&f.map, &s));
    CHECK(sok_validate(&f.map, &s));
    CHECK(sok_move(&f.map, &s, SOK_LEFT));
    CHECK(sok_move(&f.map, &s, SOK_DOWN));
    CHECK(sok_undo(&f.map, &s));
    CHECK(sok_move(&f.map, &s, SOK_LEFT));
    CHECK(s.undo_count == 2 && s.undo[1] == SOK_LEFT);
    CHECK(sok_undo(&f.map, &s));
    CHECK(sok_undo(&f.map, &s));
    CHECK(board_equal(&f.map, &s, &after_two));
    CHECK(!sok_undo(&f.map, &s));

    SokState other;
    CHECK(sok_init(&sok_maps[1], &other));
    SokState saved_other = other;
    CHECK(sok_move(&f.map, &s, SOK_DOWN));
    CHECK(state_equal(&sok_maps[1], &other, &saved_other));
    CHECK(other.undo_count == 0);
    CHECK(sok_init(&f.map, &s)); /* INIT is local and empties history. */
    CHECK(s.moves == 0 && s.pushes == 0 && s.undo_count == 0);
    CHECK(state_equal(&sok_maps[1], &other, &saved_other));
}

static void test_validation_and_overflow(void)
{
    Fixture f;
    fixture(&f, 7, 4, "#######" "#@$.  #" "#     #" "#######");
    SokState s;
    CHECK(sok_init(&f.map, &s));
    CHECK(sok_move(&f.map, &s, SOK_RIGHT));
    CHECK(sok_move(&f.map, &s, SOK_RIGHT));
    CHECK(sok_move(&f.map, &s, SOK_DOWN));
    CHECK(sok_validate(&f.map, &s));
    SokState bad = s;
    bad.player = 0;
    CHECK(!sok_validate(&f.map, &bad));
    bad = s; bad.crates[0] = s.player;
    CHECK(!sok_validate(&f.map, &bad));
    bad = s; bad.crates[0] = (uint16_t)(f.map.width * f.map.height);
    CHECK(!sok_validate(&f.map, &bad));
    bad = s; bad.pushes = bad.moves + 1u;
    CHECK(!sok_validate(&f.map, &bad));
    bad = s; bad.undo_count = 6;
    CHECK(!sok_validate(&f.map, &bad));
    bad = s; bad.undo[0] = 8;
    CHECK(!sok_validate(&f.map, &bad));
    bad = s; bad.undo[2] |= SOK_UNDO_PUSH;
    CHECK(!sok_validate(&f.map, &bad));
    bad = s; bad.undo[0] &= 3u; /* Invented walk cannot explain crate shift. */
    CHECK(!sok_validate(&f.map, &bad));
    bad = s; bad.undo[4] = 1;
    CHECK(!sok_validate(&f.map, &bad));
    bad = s; bad.pushes = 1;
    CHECK(!sok_validate(&f.map, &bad));
    bad = s; bad.moves = 2;
    CHECK(!sok_validate(&f.map, &bad));
    CHECK(sok_init(&f.map, &bad));
    bad.player = 15;
    CHECK(!sok_validate(&f.map, &bad));
    CHECK(sok_init(&f.map, &bad));
    bad.crates[0] = 10;
    CHECK(!sok_validate(&f.map, &bad));

    CHECK(sok_init(&f.map, &s));
    s.moves = UINT32_MAX;
    SokState before = s;
    CHECK(!sok_move(&f.map, &s, SOK_RIGHT));
    CHECK(state_equal(&f.map, &s, &before));
    CHECK(!sok_move(&f.map, &s, SOK_DOWN));
    CHECK(state_equal(&f.map, &s, &before));
    s.moves = UINT32_MAX - 1u;
    s.pushes = UINT32_MAX - 1u;
    CHECK(sok_move(&f.map, &s, SOK_RIGHT));
    CHECK(s.moves == UINT32_MAX && s.pushes == UINT32_MAX);
    CHECK(sok_validate(&f.map, &s));
    CHECK(!sok_move(&f.map, &s, SOK_RIGHT));
    CHECK(sok_undo(&f.map, &s));
    CHECK(s.moves == UINT32_MAX - 1u && s.pushes == UINT32_MAX - 1u);

    fixture(&f, 7, 3, "#######" "#@$$..#" "#######");
    CHECK(sok_init(&f.map, &s));
    s.crates[1] = s.crates[0];
    CHECK(!sok_validate(&f.map, &s));
    CHECK(!sok_validate(NULL, &s));
    CHECK(!sok_validate(&f.map, NULL));
    CHECK(!sok_init(NULL, &s));
    CHECK(!sok_init(&f.map, NULL));
    CHECK(!sok_move(NULL, &s, SOK_UP));
    CHECK(!sok_undo(&f.map, NULL));
    CHECK(!sok_solved(NULL, &s));
}

static uint32_t random_state = UINT32_C(0x918357ab);

static uint32_t random_number(void)
{
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return random_state;
}

static void test_all_maps_random_walk_undo(void)
{
    unsigned legal = 0;
    unsigned pushes = 0;
    for (unsigned level = 0; level < SOK_LEVEL_COUNT; ++level) {
        const SokMap *map = &sok_maps[level];
        SokState s;
        CHECK(sok_init(map, &s));
        CHECK(sok_validate(map, &s));
        for (unsigned step = 0; step < 2000; ++step) {
            SokState before = s;
            SokDirection direction = (SokDirection)(random_number() & 3u);
            if (sok_move(map, &s, direction)) {
                ++legal;
                if (s.pushes != before.pushes) ++pushes;
                CHECK(sok_validate(map, &s));
                SokState moved = s;
                CHECK(sok_undo(map, &s));
                CHECK(board_equal(map, &s, &before));
                CHECK(sok_validate(map, &s));
                /* Resume the actual random path, retaining its valid history. */
                s = moved;
                if ((random_number() & 15u) == 0) {
                    CHECK(sok_undo(map, &s));
                    CHECK(sok_validate(map, &s));
                }
            } else {
                CHECK(state_equal(map, &s, &before));
            }
        }
        while (s.undo_count > 0) {
            CHECK(sok_undo(map, &s));
            CHECK(sok_validate(map, &s));
        }
    }
    CHECK(legal > 10000 && pushes > 100);
    printf("engine random property: %u successful moves, %u pushes across 60 maps\n",
        legal, pushes);
}

int main(void)
{
    test_walk_and_completion();
    test_push_and_goal_preservation();
    test_blocked_and_boundaries();
    test_history_limit_and_branch();
    test_validation_and_overflow();
    test_all_maps_random_walk_undo();
    printf("engine: %u assertions passed; state=%zu bytes, undo=%zu bytes\n",
        assertions, sizeof(SokState), sizeof(((SokState *)0)->undo));
    return EXIT_SUCCESS;
}

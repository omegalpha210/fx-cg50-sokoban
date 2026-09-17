#include "game.h"

#include <limits.h>
#include <stddef.h>
#include <string.h>

static bool map_shape_valid(const SokMap *map)
{
    return map != NULL && map->terrain != NULL && map->crates != NULL &&
        map->width != 0 && map->height != 0 && map->crate_count != 0 &&
        map->crate_count <= SOK_MAX_CRATES &&
        (unsigned)map->width * map->height <= SOK_MAX_CELLS;
}

static bool traversable(const SokMap *map, uint16_t position)
{
    if ((unsigned)position >= (unsigned)map->width * map->height) return false;
    uint8_t terrain = sok_map_terrain(map, position);
    return terrain == SOK_FLOOR || terrain == SOK_GOAL;
}

static bool adjacent(const SokMap *map, uint16_t position,
    SokDirection direction, uint16_t *result)
{
    if ((unsigned)position >= (unsigned)map->width * map->height) return false;
    unsigned x = position % map->width;
    unsigned y = position / map->width;
    switch (direction) {
    case SOK_UP:
        if (y == 0) return false;
        *result = (uint16_t)(position - map->width);
        return true;
    case SOK_RIGHT:
        if (x + 1u == map->width) return false;
        *result = (uint16_t)(position + 1u);
        return true;
    case SOK_DOWN:
        if (y + 1u == map->height) return false;
        *result = (uint16_t)(position + map->width);
        return true;
    case SOK_LEFT:
        if (x == 0) return false;
        *result = (uint16_t)(position - 1u);
        return true;
    }
    return false;
}

int sok_crate_at(const SokMap *map, const SokState *state, uint16_t position)
{
    if (map == NULL || state == NULL || map->crate_count > SOK_MAX_CRATES)
        return -1;
    for (unsigned i = 0; i < map->crate_count; ++i)
        if (state->crates[i] == position) return (int)i;
    return -1;
}

static bool snapshot_valid(const SokMap *map, const SokState *state)
{
    if (!traversable(map, state->player) || state->pushes > state->moves)
        return false;
    for (unsigned i = 0; i < map->crate_count; ++i) {
        uint16_t position = state->crates[i];
        if (!traversable(map, position) || position == state->player)
            return false;
        for (unsigned j = 0; j < i; ++j)
            if (position == state->crates[j]) return false;
        /* With no pushes, every crate must still occupy an original square. */
        if (state->pushes == 0) {
            bool initial = false;
            for (unsigned j = 0; j < map->crate_count; ++j)
                if (position == map->crates[j]) initial = true;
            if (!initial) return false;
        }
    }
    return state->moves != 0 || state->player == map->player;
}

/* The transition primitive does not modify undo history. All checks precede
 * mutation, including counter overflow, so blocked inputs are exact no-ops. */
static bool forward(const SokMap *map, SokState *state,
    SokDirection direction, bool *pushed)
{
    uint16_t next;
    uint16_t beyond = 0;
    if ((unsigned)direction > SOK_LEFT || state->moves == UINT32_MAX ||
        state->pushes > state->moves ||
        !adjacent(map, state->player, direction, &next) ||
        !traversable(map, next)) return false;
    int crate = sok_crate_at(map, state, next);
    if (crate >= 0) {
        if (state->pushes == UINT32_MAX ||
            !adjacent(map, next, direction, &beyond) ||
            !traversable(map, beyond) ||
            sok_crate_at(map, state, beyond) >= 0) return false;
    }
    state->player = next;
    ++state->moves;
    *pushed = crate >= 0;
    if (crate >= 0) {
        state->crates[(unsigned)crate] = beyond;
        ++state->pushes;
    }
    return true;
}

static bool reverse(const SokMap *map, SokState *state, uint8_t record)
{
    uint16_t previous;
    uint16_t crate_position = 0;
    int crate = -1;
    SokDirection direction = (SokDirection)(record & 3u);
    SokDirection opposite = (SokDirection)(((unsigned)direction + 2u) & 3u);
    if (record > 7u || state->moves == 0 ||
        !adjacent(map, state->player, opposite, &previous) ||
        !traversable(map, previous) ||
        sok_crate_at(map, state, previous) >= 0) return false;
    if ((record & SOK_UNDO_PUSH) != 0) {
        if (state->pushes == 0 ||
            !adjacent(map, state->player, direction, &crate_position))
            return false;
        crate = sok_crate_at(map, state, crate_position);
        if (crate < 0) return false;
    }
    if (crate >= 0) {
        state->crates[(unsigned)crate] = state->player;
        --state->pushes;
    }
    state->player = previous;
    --state->moves;
    return true;
}

bool sok_init(const SokMap *map, SokState *state)
{
    if (!map_shape_valid(map) || state == NULL) return false;
    SokState initial = {0};
    initial.player = map->player;
    for (unsigned i = 0; i < map->crate_count; ++i)
        initial.crates[i] = map->crates[i];
    if (!snapshot_valid(map, &initial)) return false;
    *state = initial;
    return true;
}

bool sok_move(const SokMap *map, SokState *state, SokDirection direction)
{
    if (!map_shape_valid(map) || state == NULL ||
        state->undo_count > SOK_UNDO_LIMIT) return false;
    bool pushed;
    if (!forward(map, state, direction, &pushed)) return false;
    if (state->undo_count == SOK_UNDO_LIMIT) {
        memmove(state->undo, state->undo + 1, SOK_UNDO_LIMIT - 1u);
        --state->undo_count;
    }
    state->undo[state->undo_count++] =
        (uint8_t)((unsigned)direction | (pushed ? SOK_UNDO_PUSH : 0u));
    return true;
}

bool sok_undo(const SokMap *map, SokState *state)
{
    if (!map_shape_valid(map) || state == NULL || state->undo_count == 0 ||
        state->undo_count > SOK_UNDO_LIMIT) return false;
    if (!reverse(map, state, state->undo[state->undo_count - 1u])) return false;
    state->undo[--state->undo_count] = 0;
    return true;
}

unsigned sok_on_goals(const SokMap *map, const SokState *state)
{
    if (!map_shape_valid(map) || state == NULL) return 0;
    unsigned count = 0;
    for (unsigned i = 0; i < map->crate_count; ++i)
        if ((unsigned)state->crates[i] < (unsigned)map->width * map->height &&
            sok_map_terrain(map, state->crates[i]) == SOK_GOAL) ++count;
    return count;
}

bool sok_solved(const SokMap *map, const SokState *state)
{
    return map_shape_valid(map) && state != NULL &&
        sok_on_goals(map, state) == map->crate_count;
}

bool sok_validate(const SokMap *map, const SokState *state)
{
    if (!map_shape_valid(map) || state == NULL ||
        state->undo_count > SOK_UNDO_LIMIT ||
        state->undo_count > state->moves || !snapshot_valid(map, state))
        return false;
    for (unsigned i = state->undo_count; i < SOK_UNDO_LIMIT; ++i)
        if (state->undo[i] != 0) return false;

    SokState replay = *state;
    for (unsigned i = state->undo_count; i > 0; --i) {
        if (!reverse(map, &replay, state->undo[i - 1u]) ||
            !snapshot_valid(map, &replay)) return false;
    }
    for (unsigned i = 0; i < state->undo_count; ++i) {
        bool pushed;
        if (!forward(map, &replay, (SokDirection)(state->undo[i] & 3u),
            &pushed) || pushed != ((state->undo[i] & SOK_UNDO_PUSH) != 0))
            return false;
    }
    if (replay.player != state->player || replay.moves != state->moves ||
        replay.pushes != state->pushes) return false;
    for (unsigned i = 0; i < map->crate_count; ++i)
        if (replay.crates[i] != state->crates[i]) return false;
    return true;
}

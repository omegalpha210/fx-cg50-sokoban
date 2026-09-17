#ifndef SOKOBAN_GAME_H
#define SOKOBAN_GAME_H

#include "maps.h"

#include <stdbool.h>
#include <stdint.h>

#define SOK_UNDO_LIMIT 5u
#define SOK_UNDO_PUSH 4u

typedef enum SokDirection {
    SOK_UP = 0,
    SOK_RIGHT = 1,
    SOK_DOWN = 2,
    SOK_LEFT = 3
} SokDirection;

/* Undo records use bits 0..1 for direction and bit 2 for a push.
 * Oldest is undo[0]; there are no pointers or duplicated terrain boards.
 * Serialize fields explicitly: native structure padding is not a file format. */
typedef struct SokState {
    uint16_t player;
    uint16_t crates[SOK_MAX_CRATES];
    uint32_t moves;
    uint32_t pushes;
    uint8_t undo[SOK_UNDO_LIMIT];
    uint8_t undo_count;
} SokState;

typedef SokState SokGame;

bool sok_init(const SokMap *map, SokState *state);
bool sok_move(const SokMap *map, SokState *state, SokDirection direction);
bool sok_undo(const SokMap *map, SokState *state);
unsigned sok_on_goals(const SokMap *map, const SokState *state);
bool sok_solved(const SokMap *map, const SokState *state);
int sok_crate_at(const SokMap *map, const SokState *state, uint16_t position);

/* Rejects invalid positions, overlapping occupants, counter inconsistencies,
 * and history which cannot be reversed and legally replayed to this state. */
bool sok_validate(const SokMap *map, const SokState *state);

#endif

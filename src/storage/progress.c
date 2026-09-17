#include "storage.h"
#include <string.h>

void sok_progress_init(SokProgress *progress)
{
    if(progress) memset(progress, 0, sizeof(*progress));
}

bool sok_progress_resume(const SokProgress *progress, unsigned level_id,
    SokState *state)
{
    const SokMap *map = sok_get_map(level_id);
    if(!progress || !state || !map) return false;
    if(progress->in_progress[level_id - 1u]) {
        *state = progress->levels[level_id - 1u];
        return true;
    }
    return sok_init(map, state);
}

bool sok_progress_checkpoint(SokProgress *progress, unsigned level_id,
    const SokState *state, bool completed)
{
    const SokMap *map = sok_get_map(level_id);
    if(!progress || !state || !map || !sok_validate(map, state)
        || completed != sok_solved(map, state)) return false;
    unsigned index = level_id - 1u;
    progress->levels[index] = *state;
    progress->in_progress[index] = completed ? 0u : 1u;
    if(completed) progress->cleared[index] = 1;
    progress->dirty = true;
    return true;
}

bool sok_progress_reset_level(SokProgress *progress, unsigned level_id)
{
    const SokMap *map = sok_get_map(level_id);
    if(!progress || !map) return false;
    unsigned index = level_id - 1u;
    if(!sok_init(map, &progress->levels[index])) return false;
    progress->in_progress[index] = 1;
    progress->dirty = true;
    return true;
}

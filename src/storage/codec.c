#include "storage.h"
#include <string.h>

static const uint8_t magic[8] = {'S','O','K','O','B','A','N',0};

static uint16_t get16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | (uint16_t)((uint16_t)p[1] << 8));
}

static uint32_t get32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8)
        | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void put16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static void put32(uint8_t *p, uint32_t value)
{
    for(unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(value >> (8u * i));
}

uint32_t sok_save_crc32(const uint8_t *data, size_t length)
{
    uint32_t crc = UINT32_MAX;
    for(size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for(unsigned bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ ((0u - (crc & 1u)) & UINT32_C(0xedb88320));
    }
    return ~crc;
}

static bool valid_progress(const SokProgress *progress, size_t *size)
{
    if(!progress) return false;
    *size = SOK_SAVE_HEADER_SIZE + 4u + 2u * SOK_LEVEL_COUNT;
    for(unsigned i = 0; i < SOK_LEVEL_COUNT; ++i) {
        if(progress->in_progress[i] > 1u || progress->cleared[i] > 1u)
            return false;
        if(!progress->in_progress[i]) continue;
        const SokState *state = &progress->levels[i];
        if(!sok_validate(&sok_maps[i], state) || sok_solved(&sok_maps[i], state))
            return false;
        *size += 12u + 2u * sok_maps[i].crate_count + state->undo_count;
    }
    return true;
}

bool sok_save_encode(const SokProgress *progress, uint32_t generation,
    uint8_t *out, size_t capacity, size_t *length)
{
    size_t required;
    if(!out || !length || generation == 0 || !valid_progress(progress, &required)
        || required > capacity || required > SOK_SAVE_MAX_SIZE) return false;
    memset(out, 0, SOK_SAVE_HEADER_SIZE);
    memcpy(out, magic, sizeof(magic));
    put16(out + 8, 1);
    put16(out + 10, SOK_SAVE_HEADER_SIZE);
    put32(out + 12, (uint32_t)required);
    put32(out + 16, generation);
    memcpy(out + 20, sok_map_pack_hash, 32);
    put16(out + 52, SOK_LEVEL_COUNT);
    uint8_t *p = out + SOK_SAVE_HEADER_SIZE;
    for(unsigned i = 0; i < SOK_LEVEL_COUNT; ++i) {
        *p++ = (uint8_t)(i + 1u);
        *p++ = (uint8_t)(progress->in_progress[i] | (progress->cleared[i] << 1));
        if(!progress->in_progress[i]) continue;
        const SokState *state = &progress->levels[i];
        put16(p, state->player); p += 2;
        *p++ = sok_maps[i].crate_count;
        *p++ = state->undo_count;
        put32(p, state->moves); p += 4;
        put32(p, state->pushes); p += 4;
        for(unsigned c = 0; c < sok_maps[i].crate_count; ++c) {
            put16(p, state->crates[c]); p += 2;
        }
        memcpy(p, state->undo, state->undo_count); p += state->undo_count;
    }
    put32(p, sok_save_crc32(out, required - 4u));
    *length = required;
    return true;
}

/* Validation pass uses just one compact state; only after it succeeds is
 * the immutable buffer decoded a second time into caller RAM. */
static bool records(const uint8_t *data, size_t length, SokProgress *out)
{
    size_t offset = SOK_SAVE_HEADER_SIZE, end = length - 4u;
    for(unsigned i = 0; i < SOK_LEVEL_COUNT; ++i) {
        if(end - offset < 2u || data[offset] != i + 1u) return false;
        uint8_t flags = data[offset + 1u];
        offset += 2u;
        if(flags > 3u) return false;
        SokState state = {0};
        if(flags & 1u) {
            if(end - offset < 12u) return false;
            state.player = get16(data + offset);
            unsigned count = data[offset + 2u];
            state.undo_count = data[offset + 3u];
            state.moves = get32(data + offset + 4u);
            state.pushes = get32(data + offset + 8u);
            offset += 12u;
            if(count != sok_maps[i].crate_count || state.undo_count > SOK_UNDO_LIMIT
                || end - offset < 2u * count + state.undo_count) return false;
            for(unsigned c = 0; c < count; ++c) {
                state.crates[c] = get16(data + offset); offset += 2u;
            }
            memcpy(state.undo, data + offset, state.undo_count);
            offset += state.undo_count;
            if(!out && (!sok_validate(&sok_maps[i], &state)
                || sok_solved(&sok_maps[i], &state))) return false;
        }
        if(out) {
            out->levels[i] = state;
            out->in_progress[i] = flags & 1u;
            out->cleared[i] = (flags >> 1) & 1u;
        }
    }
    return offset == end;
}

bool sok_save_decode(const uint8_t *data, size_t length, SokProgress *out)
{
    if(!data || length < SOK_SAVE_HEADER_SIZE + 4u || length > SOK_SAVE_MAX_SIZE
        || memcmp(data, magic, sizeof(magic)) != 0 || get16(data + 8) != 1
        || get16(data + 10) != SOK_SAVE_HEADER_SIZE || get32(data + 12) != length
        || get32(data + 16) == 0 || memcmp(data + 20, sok_map_pack_hash, 32) != 0
        || get16(data + 52) != SOK_LEVEL_COUNT || get16(data + 54) != 0
        || get32(data + length - 4u) != sok_save_crc32(data, length - 4u)
        || !records(data, length, NULL)) return false;
    if(out) {
        (void)records(data, length, out);
        out->generation = get32(data + 16);
        out->dirty = false;
    }
    return true;
}

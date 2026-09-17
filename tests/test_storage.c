#include "storage.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef enum Fault {
    F_NONE, F_OPEN_WRITE, F_WRITE, F_CLOSE_WRITE, F_OPEN_VERIFY,
    F_READ_VERIFY, F_CLOSE_VERIFY, F_CORRUPT_VERIFY, F_READ_ALL
} Fault;

typedef struct FakeFS {
    uint8_t bytes[2][SOK_SAVE_MAX_SIZE + 1];
    size_t length[2], offset;
    bool exists[2], opened, writing, wrote;
    unsigned slot, opens, closes;
    Fault fault;
} FakeFS;

static FakeFS fs;
static SokProgress progress, loaded, before;
static uint8_t encoded[SOK_SAVE_MAX_SIZE + 1], copy[SOK_SAVE_MAX_SIZE + 1];

static int fake_open(void *context, unsigned slot, bool writing)
{
    FakeFS *f = context;
    assert(slot < 2 && !f->opened);
    if(writing && f->fault == F_OPEN_WRITE) return SOK_IO_ERROR;
    if(!writing && f->wrote && f->fault == F_OPEN_VERIFY) return SOK_IO_ERROR;
    if(!writing && !f->exists[slot]) return SOK_IO_ABSENT;
    f->slot = slot;
    f->writing = writing;
    f->offset = 0;
    f->opened = true;
    ++f->opens;
    if(writing) {
        f->length[slot] = 0;
        f->exists[slot] = true;
        f->wrote = true;
    }
    return 0; /* Descriptor zero is valid. */
}

static ptrdiff_t fake_read(void *context, int fd, void *buffer, size_t length)
{
    FakeFS *f = context;
    assert(fd == 0 && f->opened && !f->writing);
    if(f->fault == F_READ_ALL || (f->wrote && f->fault == F_READ_VERIFY)) return -1;
    size_t remaining = f->length[f->slot] - f->offset;
    if(length > remaining) length = remaining;
    if(length > 37) length = 37; /* Exercise short reads. */
    memcpy(buffer, f->bytes[f->slot] + f->offset, length);
    if(length && f->wrote && f->fault == F_CORRUPT_VERIFY)
        ((uint8_t *)buffer)[0] ^= 1u;
    f->offset += length;
    return (ptrdiff_t)length;
}

static ptrdiff_t fake_write(void *context, int fd, const void *buffer, size_t length)
{
    FakeFS *f = context;
    assert(fd == 0 && f->opened && f->writing);
    if(f->fault == F_WRITE && f->offset > 0) return -1;
    if(length > 53) length = 53; /* Exercise short writes. */
    assert(f->offset + length <= sizeof(f->bytes[0]));
    memcpy(f->bytes[f->slot] + f->offset, buffer, length);
    f->offset += length;
    f->length[f->slot] = f->offset;
    return (ptrdiff_t)length;
}

static int fake_close(void *context, int fd)
{
    FakeFS *f = context;
    assert(fd == 0 && f->opened);
    f->opened = false;
    ++f->closes;
    return (f->writing && f->fault == F_CLOSE_WRITE)
        || (!f->writing && f->wrote && f->fault == F_CLOSE_VERIFY) ? -1 : 0;
}

static const SokStorageIO io = {&fs, fake_open, fake_read, fake_write, fake_close};

static void prepare(void)
{
    memset(&fs, 0, sizeof(fs));
    sok_progress_init(&progress);
    for(unsigned id = 1; id <= SOK_LEVEL_COUNT; ++id) {
        SokState state;
        assert(sok_progress_resume(&progress, id, &state));
        assert(sok_progress_checkpoint(&progress, id, &state, false));
    }
}

static bool one_move(unsigned id, SokState *state)
{
    for(unsigned d = 0; d < 4; ++d) {
        SokState candidate = *state;
        if(sok_move(sok_get_map(id), &candidate, (SokDirection)d)
            && !sok_solved(sok_get_map(id), &candidate)) {
            *state = candidate;
            return true;
        }
    }
    return false;
}

static void recheck_crc(uint8_t *data, size_t size)
{
    uint32_t crc = sok_save_crc32(data, size - 4u);
    for(unsigned i = 0; i < 4; ++i) data[size - 4u + i] = (uint8_t)(crc >> (8u * i));
}

static void rejected(size_t size)
{
    before = loaded;
    assert(!sok_save_decode(copy, size, &loaded));
    assert(memcmp(&before, &loaded, sizeof(before)) == 0);
}

static void test_codec(void)
{
    prepare();
    assert(sok_save_crc32((const uint8_t *)"123456789", 9) == UINT32_C(0xcbf43926));
    for(unsigned id = 1; id <= SOK_LEVEL_COUNT; ++id) {
        for(unsigned count = 0; count < 8; ++count)
            assert(one_move(id, &progress.levels[id - 1u]));
        progress.cleared[id - 1u] = (uint8_t)(id % 2u);
    }
    size_t size;
    assert(sok_save_encode(&progress, 7, encoded, sizeof(encoded), &size));
    assert(size <= SOK_SAVE_MAX_SIZE);
    assert(!sok_save_encode(&progress, 7, copy, size - 1u, &size));
    assert(sok_save_decode(encoded, size, &loaded));
    assert(loaded.generation == 7 && !loaded.dirty);
    for(unsigned id = 1; id <= SOK_LEVEL_COUNT; ++id) {
        assert(loaded.in_progress[id - 1u]);
        assert(loaded.cleared[id - 1u] == progress.cleared[id - 1u]);
        assert(memcmp(&loaded.levels[id - 1u], &progress.levels[id - 1u], sizeof(SokState)) == 0);
        SokState a = loaded.levels[id - 1u], b = progress.levels[id - 1u];
        for(unsigned count = 0; count < 5; ++count) {
            assert(sok_undo(sok_get_map(id), &a));
            assert(sok_undo(sok_get_map(id), &b));
            assert(memcmp(&a, &b, sizeof(a)) == 0);
        }
        assert(!sok_undo(sok_get_map(id), &a));
    }
    memcpy(copy, encoded, size);
    for(size_t truncated = 0; truncated < size; ++truncated) rejected(truncated);
    copy[18] ^= 1u; rejected(size); /* CRC error. */
    memcpy(copy, encoded, size); copy[20] ^= 1u;
    recheck_crc(copy, size); rejected(size); /* Rechecksummed wrong map pack. */
    memcpy(copy, encoded, size); copy[8] = 2;
    recheck_crc(copy, size); rejected(size);
    memcpy(copy, encoded, size); copy[56] = 2;
    recheck_crc(copy, size); rejected(size); /* Wrong level ID. */
    memcpy(copy, encoded, size); copy[57] = 4;
    recheck_crc(copy, size); rejected(size);
    memcpy(copy, encoded, size); copy[58] = 255; copy[59] = 255;
    recheck_crc(copy, size); rejected(size); /* Player out of board. */
    memcpy(copy, encoded, size); copy[60] = (uint8_t)(sok_maps[0].crate_count - 1u);
    recheck_crc(copy, size); rejected(size);
    memcpy(copy, encoded, size); copy[61] = 6;
    recheck_crc(copy, size); rejected(size);
    memcpy(copy, encoded, size); memset(copy + 66, 255, 4);
    recheck_crc(copy, size); rejected(size); /* Pushes exceed moves. */
    memcpy(copy, encoded, size); copy[72] = copy[70]; copy[73] = copy[71];
    recheck_crc(copy, size); rejected(size); /* Duplicate crate. */
    size_t undo_offset = 70u + 2u * sok_maps[0].crate_count;
    memcpy(copy, encoded, size); copy[undo_offset] = 8;
    recheck_crc(copy, size); rejected(size); /* Unknown undo bits. */
    memcpy(copy, encoded, size); copy[undo_offset + 4u] ^= SOK_UNDO_PUSH;
    recheck_crc(copy, size); rejected(size); /* Impossible reverse push. */
    printf("storage codec: 60-level roundtrip + undo, %zu bytes with 5 undo records each\n", size);
}

static void save_ok(void)
{
    fs.wrote = false;
    fs.fault = F_NONE;
    assert(sok_storage_save_io(&progress, &io));
    assert(!progress.dirty && !fs.opened && fs.opens == fs.closes);
}

static void test_slots(void)
{
    prepare();
    assert(sok_storage_load_io(&loaded, &io) == SOK_LOAD_NEW);
    save_ok();
    assert(progress.generation == 1 && fs.exists[0] && !fs.exists[1]);
    assert(one_move(1, &progress.levels[0])); progress.dirty = true;
    save_ok();
    assert(progress.generation == 2 && fs.exists[1]);
    fs.wrote = false;
    assert(sok_storage_load_io(&loaded, &io) == SOK_LOAD_OK);
    assert(loaded.generation == 2 && loaded.levels[0].moves == 1);
    fs.bytes[1][fs.length[1] - 1u] ^= 1u;
    assert(sok_storage_load_io(&loaded, &io) == SOK_LOAD_RECOVERED);
    assert(loaded.generation == 1 && loaded.levels[0].moves == 0);
    --fs.length[0];
    assert(sok_storage_load_io(&loaded, &io) == SOK_LOAD_INVALID);
    assert(loaded.generation == 0 && !loaded.in_progress[0] && !loaded.dirty);
    fs.fault = F_READ_ALL;
    assert(sok_storage_load_io(&loaded, &io) == SOK_LOAD_IO_ERROR);
    assert(!fs.opened && fs.opens == fs.closes);
}

static void test_failed_saves(void)
{
    const Fault failures[] = {F_OPEN_WRITE, F_WRITE, F_CLOSE_WRITE, F_OPEN_VERIFY,
        F_READ_VERIFY, F_CLOSE_VERIFY, F_CORRUPT_VERIFY, F_READ_ALL};
    for(unsigned i = 0; i < sizeof(failures) / sizeof(failures[0]); ++i) {
        prepare(); save_ok();
        assert(one_move(1, &progress.levels[0])); progress.dirty = true;
        save_ok(); /* Slot B generation 2 is now the last valid save. */
        size_t valid_size = fs.length[1];
        memcpy(copy, fs.bytes[1], valid_size);
        assert(one_move(2, &progress.levels[1])); progress.dirty = true;
        before = progress;
        fs.wrote = false;
        fs.fault = failures[i];
        assert(!sok_storage_save_io(&progress, &io));
        assert(memcmp(&progress, &before, sizeof(progress)) == 0);
        assert(progress.dirty && progress.generation == 2);
        assert(fs.length[1] == valid_size && memcmp(copy, fs.bytes[1], valid_size) == 0);
        assert(!fs.opened && fs.opens == fs.closes);
        save_ok(); /* Explicit retry preserves all RAM progress. */
        fs.wrote = false;
        assert(sok_storage_load_io(&loaded, &io) == SOK_LOAD_OK);
        assert(loaded.levels[0].moves == 1 && loaded.levels[1].moves == 1);
    }
    prepare(); save_ok();
    unsigned opens = fs.opens;
    assert(sok_storage_save_io(&progress, &io));
    assert(fs.opens == opens); /* Clean boundaries do not touch flash. */
    progress.generation = UINT32_MAX; progress.dirty = true;
    before = progress;
    assert(!sok_storage_save_io(&progress, &io));
    assert(memcmp(&before, &progress, sizeof(progress)) == 0);
}

static void test_independence(void)
{
    prepare();
    for(unsigned i = 0; i < 60; ++i) assert(one_move(i + 1u, &progress.levels[i]));
    progress.cleared[0] = 1;
    before = progress;
    assert(sok_progress_reset_level(&progress, 1));
    assert(progress.cleared[0] && progress.in_progress[0]);
    assert(progress.levels[0].moves == 0 && progress.levels[0].undo_count == 0);
    for(unsigned i = 1; i < 60; ++i) {
        assert(memcmp(&before.levels[i], &progress.levels[i], sizeof(SokState)) == 0);
        assert(before.in_progress[i] == progress.in_progress[i]);
    }
    save_ok();
    fs.wrote = false;
    assert(sok_storage_load_io(&loaded, &io) == SOK_LOAD_OK);
    SokState resumed;
    assert(sok_progress_resume(&loaded, 1, &resumed));
    assert(resumed.moves == 0 && loaded.cleared[0]);
    assert(!sok_progress_resume(&loaded, 0, &resumed));
    assert(!sok_progress_reset_level(&progress, 61));
    assert(!sok_progress_checkpoint(&progress, 1, &resumed, true));
}

int main(void)
{
    test_codec();
    test_slots();
    test_failed_saves();
    test_independence();
    printf("storage tests passed; progress=%zu bytes, codec workspace=%u bytes\n",
        sizeof(SokProgress), (unsigned)SOK_SAVE_MAX_SIZE);
    return 0;
}

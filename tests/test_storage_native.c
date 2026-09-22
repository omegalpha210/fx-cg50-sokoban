#include "storage.h"
#include <gint/gint.h>
#include <gint/bfile.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint8_t files[2][SOK_SAVE_MAX_SIZE + 1u];
static int sizes[2], position, slot;
static bool exists[2], opened, writing, os_world;
static unsigned switches, calls, failed_closes;
static bool fail_write, fail_readback, fail_create, wrote;
static unsigned fail_close_write;
static unsigned injected_call;
static SokProgress progress, loaded, before;
static uint8_t backup[SOK_SAVE_MAX_SIZE];

static bool native_call(void)
{
    assert(os_world);
    ++calls;
    return calls==injected_call;
}

static int path_slot(const uint16_t *path)
{
    const uint16_t *expected[2] = {u"\\\\fls0\\SOKO_A.dat", u"\\\\fls0\\SOKO_B.dat"};
    for(int candidate = 0; candidate < 2; ++candidate) {
        unsigned i = 0;
        while(path[i] == expected[candidate][i] && path[i]) ++i;
        if(!path[i] && !expected[candidate][i]) return candidate;
    }
    assert(0 && "Unexpected path: must stay in SOKOBAN namespace");
    return -1;
}

int gint_world_switch(gint_call_t call)
{
    assert(!os_world);
    os_world = true;
    ++switches;
    int result = call.function(call.argument);
    os_world = false;
    return result;
}

int BFile_Remove(const uint16_t *path)
{
    if(native_call())return -5;
    assert(!opened);
    int id = path_slot(path);
    bool had_file = exists[id];
    exists[id] = false;
    sizes[id] = 0;
    return had_file ? 0 : BFile_EntryNotFound;
}

int BFile_Create(const uint16_t *path, int type, int *size)
{
    if(native_call())return -5;
    assert(!opened && type == BFile_File && *size == 0);
    int id = path_slot(path);
    if(fail_create) return -5;
    exists[id] = true;
    sizes[id] = 0;
    return 0;
}

int BFile_Open(const uint16_t *path, int mode)
{
    if(native_call())return -5;
    assert(!opened && (mode == BFile_ReadOnly || mode == BFile_WriteOnly));
    slot = path_slot(path);
    if(!exists[slot]) return BFile_EntryNotFound;
    opened = true;
    writing = mode == BFile_WriteOnly;
    position = 0;
    return 0;
}

int BFile_Close(int fd)
{
    if(native_call())return -5;
    assert(fd == 0 && opened);
    if(writing && fail_close_write) {
        --fail_close_write;
        ++failed_closes;
        return -5;
    }
    opened = false;
    return 0;
}

int BFile_Size(int fd)
{
    if(native_call())return -5;
    assert(fd == 0 && opened);
    return sizes[slot];
}

int BFile_Write(int fd, const void *data, int size)
{
    if(native_call())return -5;
    assert(fd == 0 && opened && writing && size > 0);
    wrote = true;
    if(fail_write && position != 0) return -5;
    if(size > 41) size = 41;
    assert(position + size <= SOK_SAVE_MAX_SIZE);
    memcpy(files[slot] + position, data, (size_t)size);
    position += size;
    sizes[slot] = position;
    /* Fugue returns bytes written, not the updated offset. */
    return size;
}

int BFile_Read(int fd, void *data, int size, int offset)
{
    if(native_call())return -5;
    assert(fd == 0 && opened && !writing && offset >= 0 && size > 0);
    /* Real Fugue allows reads beyond EOF; adapter must prevent them. */
    assert(offset + size <= sizes[slot]);
    if(wrote && fail_readback) return -5;
    if(size > 29) size = 29;
    memcpy(data, files[slot] + offset, (size_t)size);
    return size;
}

static void fresh(void)
{
    assert(!opened);
    memset(files, 0, sizeof(files));
    memset(sizes, 0, sizeof(sizes));
    memset(exists, 0, sizeof(exists));
    fail_write = fail_readback = fail_create = wrote = false;
    fail_close_write = 0;
    sok_progress_init(&progress);
    assert(sok_progress_reset_level(&progress, 1));
}

static void good_save(void)
{
    unsigned previous = switches;
    wrote = false;
    assert(sok_storage_save(&progress));
    assert(switches == previous + 1 && !os_world && !opened);
    assert(!progress.dirty);
}
static void fault_sweep(void)
{
    fresh();good_save();progress.dirty=true;
    unsigned start=calls;good_save();unsigned save_calls=calls-start;
    for(unsigned point=1;point<=save_calls;point++) {
        fresh();good_save();progress.dirty=true;before=progress;
        int length=sizes[0];memcpy(backup,files[0],(size_t)length);
        injected_call=calls+point;wrote=false;
        bool ok=sok_storage_save(&progress);injected_call=0;
        assert(!os_world && !opened);
        assert(sizes[0]==length && memcmp(files[0],backup,(size_t)length)==0);
        if(ok)assert(!progress.dirty && progress.generation==2);
        else assert(memcmp(&progress,&before,sizeof(progress))==0);
        /* Retry must work after every one-shot OS failure. */
        good_save();
    }
    fresh();good_save();progress.dirty=true;good_save();wrote=false;
    start=calls;assert(sok_storage_load(&loaded)==SOK_LOAD_OK);
    unsigned load_calls=calls-start;
    for(unsigned point=1;point<=load_calls;point++) {
        injected_call=calls+point;
        SokLoadResult result=sok_storage_load(&loaded);injected_call=0;
        assert(!opened && !os_world);
        assert(result==SOK_LOAD_OK || result==SOK_LOAD_RECOVERED);
        assert(loaded.generation>=1 && loaded.generation<=2 && !loaded.dirty);
    }
    printf("native fault sweep: %u save + %u load OS-call boundaries, backup/RAM/cleanup verified\n",
        save_calls,load_calls);
}

int main(void)
{
    fresh();
    unsigned previous = switches;
    assert(sok_storage_load(&loaded) == SOK_LOAD_NEW);
    assert(switches == previous + 1 && !os_world && !opened);
    good_save();
    assert(progress.generation == 1);
    int backup_size = sizes[0];
    memcpy(backup, files[0], (size_t)backup_size);
    for(unsigned failure = 0; failure < 4; ++failure) {
        progress.dirty = true;
        before = progress;
        wrote = false;
        fail_write = failure == 0;
        fail_create = failure == 1;
        fail_readback = failure == 2;
        fail_close_write = failure == 3 ? 1u : 0u;
        previous = switches;
        assert(!sok_storage_save(&progress));
        assert(switches == previous + 1 && !os_world && !opened);
        assert(memcmp(&before, &progress, sizeof(progress)) == 0);
        assert(sizes[0] == backup_size && memcmp(backup, files[0], (size_t)backup_size) == 0);
        /* Start next test from the same last validated on-disk generation. */
        exists[1] = false;
        fail_write = fail_create = fail_readback = false;
    }
    assert(failed_closes == 1);
    /* Persistent close failure remains bounded; next boundary retries cleanup.
     * Caller can leave its retry modal because the world switch always exits. */
    wrote = false;
    fail_close_write = 2;
    previous = switches;
    assert(!sok_storage_save(&progress));
    assert(switches == previous + 1 && !os_world && opened);
    assert(progress.dirty && progress.generation == 1);
    good_save();
    assert(failed_closes == 3 && progress.generation == 3);
    wrote = false;
    previous = switches;
    assert(sok_storage_load(&loaded) == SOK_LOAD_OK);
    assert(switches == previous + 1 && !os_world && !opened);
    assert(loaded.generation == 3 && loaded.in_progress[0]);
    fault_sweep();
    printf("native storage contract passed: %u BFile calls inside %u complete OS transactions\n",
        calls, switches);
    return 0;
}

#include "storage.h"
#include <string.h>

/* Main-thread use only. A single encoded record, not duplicate progress or
 * terrain boards; no storage-owned heap allocation and no giant stack frame. */
static uint8_t workspace[SOK_SAVE_MAX_SIZE];

typedef enum SlotStatus { SLOT_ABSENT, SLOT_VALID, SLOT_INVALID, SLOT_IO } SlotStatus;
typedef struct SlotInfo {
    SlotStatus status;
    uint32_t generation;
    size_t length;
} SlotInfo;

static uint32_t generation(const uint8_t *data)
{
    return (uint32_t)data[16] | ((uint32_t)data[17] << 8)
        | ((uint32_t)data[18] << 16) | ((uint32_t)data[19] << 24);
}

static bool valid_io(const SokStorageIO *io)
{
    return io && io->open && io->read && io->write && io->close;
}

static SlotInfo probe(const SokStorageIO *io, unsigned slot)
{
    SlotInfo info = {SLOT_IO, 0, 0};
    int fd = io->open(io->context, slot, false);
    if(fd < 0) {
        if(fd == SOK_IO_ABSENT) info.status = SLOT_ABSENT;
        return info;
    }
    bool ok = true, too_long = false;
    while(info.length < sizeof(workspace)) {
        size_t remaining = sizeof(workspace) - info.length;
        ptrdiff_t n = io->read(io->context, fd, workspace + info.length, remaining);
        if(n < 0 || (size_t)n > remaining) { ok = false; break; }
        if(n == 0) break;
        info.length += (size_t)n;
    }
    if(ok && info.length == sizeof(workspace)) {
        uint8_t extra;
        ptrdiff_t n = io->read(io->context, fd, &extra, 1);
        if(n < 0) ok = false;
        else too_long = n != 0;
    }
    if(io->close(io->context, fd) != 0) ok = false;
    if(!ok) return info;
    info.status = SLOT_INVALID;
    if(!too_long && sok_save_decode(workspace, info.length, NULL)) {
        info.status = SLOT_VALID;
        info.generation = generation(workspace);
    }
    return info;
}

static int newest(const SlotInfo slots[2])
{
    if(slots[0].status != SLOT_VALID) return slots[1].status == SLOT_VALID ? 1 : -1;
    if(slots[1].status != SLOT_VALID) return 0;
    return slots[1].generation > slots[0].generation ? 1 : 0;
}

SokLoadResult sok_storage_load_io(SokProgress *progress, const SokStorageIO *io)
{
    if(!progress) return SOK_LOAD_IO_ERROR;
    sok_progress_init(progress);
    if(!valid_io(io)) return SOK_LOAD_IO_ERROR;
    SlotInfo slots[2];
    slots[0] = probe(io, 0);
    slots[1] = probe(io, 1);
    for(unsigned attempt = 0; attempt < 2; ++attempt) {
        int selected = newest(slots);
        if(selected < 0) break;
        /* Re-open the selected file: buffer was reused while probing its peer.
         * Full revalidation also handles a changed/disappearing file. */
        SlotInfo loaded = probe(io, (unsigned)selected);
        if(loaded.status == SLOT_VALID && loaded.generation == slots[selected].generation
            && sok_save_decode(workspace, loaded.length, progress)) {
            SlotStatus peer = slots[1 - selected].status;
            return attempt != 0 || peer == SLOT_INVALID || peer == SLOT_IO
                ? SOK_LOAD_RECOVERED : SOK_LOAD_OK;
        }
        slots[selected].status = loaded.status == SLOT_IO ? SLOT_IO : SLOT_INVALID;
    }
    if(slots[0].status == SLOT_IO || slots[1].status == SLOT_IO)
        return SOK_LOAD_IO_ERROR;
    return slots[0].status == SLOT_ABSENT && slots[1].status == SLOT_ABSENT
        ? SOK_LOAD_NEW : SOK_LOAD_INVALID;
}

static bool write_record(const SokStorageIO *io, unsigned slot, size_t length)
{
    int fd = io->open(io->context, slot, true);
    if(fd < 0) return false;
    bool ok = true;
    size_t offset = 0;
    while(offset < length) {
        ptrdiff_t n = io->write(io->context, fd, workspace + offset, length - offset);
        if(n <= 0 || (size_t)n > length - offset) { ok = false; break; }
        offset += (size_t)n;
    }
    if(io->close(io->context, fd) != 0) ok = false;
    return ok;
}

static bool verify_record(const SokStorageIO *io, unsigned slot, size_t length)
{
    int fd = io->open(io->context, slot, false);
    if(fd < 0) return false;
    bool ok = true;
    size_t offset = 0;
    uint8_t chunk[128];
    while(offset < length) {
        size_t count = length - offset;
        if(count > sizeof(chunk)) count = sizeof(chunk);
        ptrdiff_t n = io->read(io->context, fd, chunk, count);
        if(n <= 0 || (size_t)n > count
            || memcmp(chunk, workspace + offset, (size_t)n) != 0) {
            ok = false;
            break;
        }
        offset += (size_t)n;
    }
    if(ok && io->read(io->context, fd, chunk, 1) != 0) ok = false;
    if(io->close(io->context, fd) != 0) ok = false;
    /* Byte-identical to an encoded, structurally validated record, including
     * its CRC. Requiring EOF rejects stale trailing bytes after a short write. */
    return ok;
}

bool sok_storage_save_io(SokProgress *progress, const SokStorageIO *io)
{
    if(!progress || !valid_io(io)) return false;
    if(!progress->dirty) return true;
    SlotInfo slots[2];
    slots[0] = probe(io, 0);
    slots[1] = probe(io, 1);
    int current = newest(slots);
    /* If neither can be read, there is no known safe slot to replace. */
    if(current < 0 && (slots[0].status == SLOT_IO || slots[1].status == SLOT_IO))
        return false;
    unsigned target = current == 0 ? 1u : 0u;
    uint32_t previous = progress->generation;
    if(current >= 0 && slots[current].generation > previous)
        previous = slots[current].generation;
    if(previous == UINT32_MAX) return false;
    uint32_t next = previous + 1u;
    size_t length;
    if(!sok_save_encode(progress, next, workspace, sizeof(workspace), &length)
        || !write_record(io, target, length) || !verify_record(io, target, length))
        return false;
    progress->generation = next;
    progress->dirty = false;
    return true;
}

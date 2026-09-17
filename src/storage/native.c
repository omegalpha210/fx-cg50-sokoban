#include "storage.h"
#ifdef FXCG50
#include <gint/bfile.h>
#include <gint/gint.h>
#include <limits.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#endif

/* gint Fugue maps / to storage memory (\\fls0\\). These names are unique to
 * SOKOBAN and never access DIFF EQ files. No native IO occurs outside the
 * world-switch callback, including failure closes and readback verification. */
#ifdef FXCG50
/* Direct BFile avoids the installed Fugue wrapper's allocation and close-
 * failure leak. Its fx-CG50/Fugue return conventions were checked against
 * the installed SDK. This adapter is intentionally not a CASIOWIN adapter. */
static const uint16_t *const paths[2] = {u"\\\\fls0\\SOKO_A.dat", u"\\\\fls0\\SOKO_B.dat"};
static int position;
static int pending_close = -1;

static int native_open(void *context, unsigned slot, bool writing)
{
    (void)context;
    if(slot > 1u) return SOK_IO_ERROR;
    if(pending_close >= 0) {
        if(BFile_Close(pending_close) < 0) return SOK_IO_ERROR;
        pending_close = -1;
    }
    if(writing) {
        int rc = BFile_Remove(paths[slot]);
        if(rc < 0 && rc != BFile_EntryNotFound) return SOK_IO_ERROR;
        int size = 0;
        if(BFile_Create(paths[slot], BFile_File, &size) < 0) return SOK_IO_ERROR;
    }
    int fd = BFile_Open(paths[slot], writing ? BFile_WriteOnly : BFile_ReadOnly);
    position = 0;
    if(fd >= 0) return fd;
    return !writing && fd == BFile_EntryNotFound ? SOK_IO_ABSENT : SOK_IO_ERROR;
}

static ptrdiff_t native_read(void *context, int fd, void *buffer, size_t length)
{
    (void)context;
    int size = BFile_Size(fd);
    if(size < 0 || position > size) return -1;
    /* Fugue can report bytes beyond EOF; clamp before calling BFile_Read. */
    if(length > (size_t)(size - position)) length = (size_t)(size - position);
    if(length == 0) return 0;
    if(length > INT_MAX) return -1;
    int count = BFile_Read(fd, buffer, (int)length, position);
    if(count > 0 && (size_t)count <= length) position += count;
    return count;
}

static ptrdiff_t native_write(void *context, int fd, const void *buffer, size_t length)
{
    (void)context;
    if(length > INT_MAX) return -1;
    return BFile_Write(fd, buffer, (int)length);
}

static int native_close(void *context, int fd)
{
    (void)context;
    if(BFile_Close(fd) >= 0) return 0;
    /* Retain only a failed handle for bounded cleanup at the next boundary;
     * a close failure remains a failed transaction even if retry succeeds. */
    if(BFile_Close(fd) < 0) pending_close = fd;
    return -1;
}
#else
static const char *const paths[2] = {"/SOKO_A.dat", "/SOKO_B.dat"};

static int native_open(void *context, unsigned slot, bool writing)
{
    (void)context;
    if(slot > 1u) return SOK_IO_ERROR;
    int flags = writing ? O_WRONLY | O_CREAT | O_TRUNC : O_RDONLY;
    int fd = open(paths[slot], flags, 0600);
    if(fd >= 0) return fd;
    return !writing && errno == ENOENT ? SOK_IO_ABSENT : SOK_IO_ERROR;
}

static ptrdiff_t native_read(void *context, int fd, void *buffer, size_t length)
{
    (void)context;
    return (ptrdiff_t)read(fd, buffer, length);
}

static ptrdiff_t native_write(void *context, int fd, const void *buffer, size_t length)
{
    (void)context;
    return (ptrdiff_t)write(fd, buffer, length);
}

static int native_close(void *context, int fd)
{
    (void)context;
    return close(fd);
}
#endif

static const SokStorageIO native_io = {
    NULL, native_open, native_read, native_write, native_close
};

static int load_transaction(void *opaque)
{
    return (int)sok_storage_load_io(opaque, &native_io);
}

static int save_transaction(void *opaque)
{
    return sok_storage_save_io(opaque, &native_io) ? 1 : 0;
}

SokLoadResult sok_storage_load(SokProgress *progress)
{
#ifdef FXCG50
    return (SokLoadResult)gint_world_switch(GINT_CALL(load_transaction, (void *)progress));
#else
    return (SokLoadResult)load_transaction(progress);
#endif
}

bool sok_storage_save(SokProgress *progress)
{
#ifdef FXCG50
    return gint_world_switch(GINT_CALL(save_transaction, (void *)progress)) != 0;
#else
    return save_transaction(progress) != 0;
#endif
}

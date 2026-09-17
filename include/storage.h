#ifndef SOKOBAN_STORAGE_H
#define SOKOBAN_STORAGE_H

#include "game.h"
#include <stddef.h>

/* Compact dynamic states only: the immutable terrain remains in sok_maps. */
typedef struct SokProgress {
    SokState levels[SOK_LEVEL_COUNT];
    uint8_t in_progress[SOK_LEVEL_COUNT];
    uint8_t cleared[SOK_LEVEL_COUNT];
    uint32_t generation;
    bool dirty;
} SokProgress;

void sok_progress_init(SokProgress *progress);
bool sok_progress_resume(const SokProgress *progress, unsigned level_id,
    SokState *state);
bool sok_progress_checkpoint(SokProgress *progress, unsigned level_id,
    const SokState *state, bool completed);
bool sok_progress_reset_level(SokProgress *progress, unsigned level_id);

enum {
    SOK_SAVE_HEADER_SIZE = 56,
    SOK_SAVE_MAX_SIZE = 60 + SOK_LEVEL_COUNT * (19 + 2 * SOK_MAX_CRATES)
};

/* Explicit little-endian codec. Decode never alters output on invalid input.
 * Passing NULL as output validates a record without materializing progress. */
bool sok_save_encode(const SokProgress *progress, uint32_t generation,
    uint8_t *out, size_t capacity, size_t *length);
bool sok_save_decode(const uint8_t *data, size_t length, SokProgress *out);
uint32_t sok_save_crc32(const uint8_t *data, size_t length);

/* Backend callbacks execute synchronously in the caller's context.
 * open returns >=0 descriptor, SOK_IO_ABSENT for a missing read-only file, or
 * SOK_IO_ERROR for other failures. Writing truncates ONLY the chosen slot.
 * read/write may return short transfers; read returns 0 at EOF.
 * close must return 0 on success, and releases the descriptor on any result. */
enum { SOK_IO_ABSENT = -1, SOK_IO_ERROR = -2 };
typedef struct SokStorageIO {
    void *context;
    int (*open)(void *context, unsigned slot, bool writing);
    ptrdiff_t (*read)(void *context, int fd, void *buffer, size_t length);
    ptrdiff_t (*write)(void *context, int fd, const void *buffer, size_t length);
    int (*close)(void *context, int fd);
} SokStorageIO;

typedef enum SokLoadResult {
    SOK_LOAD_NEW,       /* Both files absent; ordinary first use. */
    SOK_LOAD_OK,
    SOK_LOAD_RECOVERED, /* A valid slot survived an unusable peer. */
    SOK_LOAD_INVALID,   /* Existing files unusable; safe empty progress. */
    SOK_LOAD_IO_ERROR   /* No usable slot and a filesystem error. */
} SokLoadResult;

/* Main-thread safe boundaries only. Shared bounded scratch makes these
 * non-reentrant. Failure leaves RAM, generation, and dirty unchanged. */
SokLoadResult sok_storage_load_io(SokProgress *progress, const SokStorageIO *io);
bool sok_storage_save_io(SokProgress *progress, const SokStorageIO *io);

/* Calculator adapter: the entire transaction, including error cleanup and
 * readback, runs in a SINGLE gint OS world switch. */
SokLoadResult sok_storage_load(SokProgress *progress);
bool sok_storage_save(SokProgress *progress);

#endif

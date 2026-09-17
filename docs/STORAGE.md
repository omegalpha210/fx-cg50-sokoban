# Progress and save transactions

All 60 levels have independent compact dynamic state. Immutable terrain is
shared from the pinned map pack. `SokProgress` holds each level's player,
crate positions, exact move/push counters, five compact undo bytes, independent
in-progress and cleared flags, and the last acknowledged generation. Clearing,
retrying, INIT, and UNDO never erase an earned cleared flag.

The app updates RAM while playing and marks changes dirty. The storage API is
called only at main-thread checkpoints; it is not reentrant and must not run
from an interrupt or timer. A clean save returns success without touching files.
No allocation occurs per move or in the project's storage code.

`sok_progress_checkpoint()` validates its state and requires its `completed`
argument to agree with the board. Completion retains the cleared flag and
removes the resume flag. Reopening that level therefore starts a fresh attempt;
a later interrupted attempt has its own resume state. INIT replaces only that
level's state and keeps its cleared flag. Serialized active states cannot be
already solved, preventing a completion modal loop after loading.

## Version 1 file format

Files are `\\fls0\SOKO_A.dat` and `\\fls0\SOKO_B.dat`. All integer fields
use explicit little-endian encoding, independent of the host or SH ABI.
No C structure, pointer, padding, or terrain board is written to disk.

| Offset | Size | Meaning |
| --- | ---: | --- |
| 0 | 8 | ASCII `SOKOBAN` followed by NUL |
| 8 | 2 | Version, currently 1 |
| 10 | 2 | Header size, 56 |
| 12 | 4 | Exact file length, including CRC |
| 16 | 4 | Generation, beginning at 1 |
| 20 | 32 | SHA-256 of the exact pinned upstream map text |
| 52 | 2 | Level count, 60 |
| 54 | 2 | Reserved, zero |
| 56 | variable | Sixty records, in ID order 1 through 60 |
| final 4 | 4 | CRC-32/ISO-HDLC of every preceding byte |

Every level record starts with its one-byte ID and one-byte flags: bit 0 is
in-progress, bit 1 is cleared, other bits are zero. An inactive record ends
there. An active record adds player cell (u16), crate count (u8), undo count
(u8), moves (u32), pushes (u32), the exact number of crate cells (u16 each),
and `undo_count` undo bytes. Undo bytes use bits 0–1 for direction and bit 2
for whether a crate was pushed. Position indices refer to the map's original
metadata width and height.

Loading checks magic, version, exact length, CRC, all 32 map-hash bytes,
generation, reserved bits, ID order, exact per-map crate count, passable and
distinct occupant cells, counters, and all undo records. The game validator
reverses and legally replays the retained history. The decoder first validates
the entire record with one small state and then materializes it, so an invalid
record never partly overwrites caller progress. It does not attempt to prove
the complete historical solution path outside the last five moves.

An empty pack record is 180 bytes. With the actual pinned pack's 1,068 crates,
all 60 states active and all five undo entries retained use at most **3,336
bytes per slot**, or 6,672 bytes for both slot files. The conservative static
codec capacity is 5,520 bytes and also bounds malicious input. `SokState` is
92 bytes and `SokProgress` is 5,648 bytes under both the host and SH layout
used here. The encoder uses the existing state directly; the decoder's
validation pass uses one 92-byte state, and readback comparison uses a
128-byte stack chunk. Compiler stack-usage reports, not these object sizes,
are the authority for individual function frames and do not prove total
runtime stack peak.

## Commit and recovery

1. Read, close, and fully validate both slots. Select the newest valid
   generation; a tied generation chooses A deterministically.
2. Choose its opposite slot for replacement. If there is no valid slot and
   any read had an I/O error, fail without replacing either file.
3. Validate and encode current RAM with the next generation. Generation
   exhaustion at `UINT32_MAX` fails explicitly instead of silently wrapping.
4. Remove/recreate only the replacement slot, write all bytes, and close it.
5. Reopen the replacement, compare every byte with the validated encoding,
   require EOF, and close it. This also verifies the encoded CRC and all
   semantic fields because the complete bytes must match.
6. Only then acknowledge the new generation and clear dirty.

Any open, create, remove, write, close, readback, or comparison failure is a
failed save. RAM, acknowledged generation, and dirty stay unchanged. The old
valid slot remains byte-for-byte intact. The UI can retry or leave explicitly
without persistence. A failed close can leave valid bytes on disk; a later
startup may legitimately recover those bytes, but the failed transaction was
never reported successful. A retry examines disk generations again.

Loading chooses the newest valid slot and revalidates it before applying it.
A corrupt peer produces `SOK_LOAD_RECOVERED`; no files produces
`SOK_LOAD_NEW`. Existing but unusable files produce `SOK_LOAD_INVALID`, and
filesystem failure without a usable peer produces `SOK_LOAD_IO_ERROR`.
The last two enter safe empty progress and require the app's startup notice.
No old coordinates are applied to a different or unknown map pack.

## Native boundary and provenance

`sok_storage_load()` and `sok_storage_save()` each wrap the **whole** native
transaction in one `gint_world_switch(GINT_CALL(...))`: probes, file creation,
all transfers, closes, cleanup, and verification all execute in the OS world.
The fx-CG50 adapter calls BFile directly with Fugue semantics. It checks
`BFile_Size()` and bounds each read because Fugue can otherwise read beyond
EOF. Fugue's write result is a byte count, so short transfers are looped.
This adapter intentionally does not support the older CASIOWIN filesystem.

The installed gint POSIX wrapper allocates descriptor/path memory and its
Fugue close implementation returns before freeing descriptor data when
`BFile_Close()` fails. Direct BFile avoids that wrapper failure path. The
adapter attempts close cleanup once immediately. If both closes fail, it
retains one handle and retries its close at the next transaction before
opening anything else; it never spins or reports the failed transaction as
successful. Persistent hardware I/O errors remain errors, and the UI's leave
choice is still available.

The code is a new implementation. Read-only references were DIFF EQ
`src/storage.c` at commit
`1a16b1b728c7c9e8a0e2491d96fde4b95cdacdbb`, installed gint `gint/gint.h`,
`gint/bfile.h`, and `src/fs/fugue/{fugue.c,fugue_open.c}`. The two-slot
preservation concept and world-switch pattern informed the design; no DIFF EQ
serialization structures, source blocks, app identifiers, or filenames were
copied. No DIFF EQ files were changed.

## Automated evidence and hardware limits

`test_storage` covers all 60 states and their retained undo history, short
transfers, missing versus unusable files, every truncation length, CRC errors,
rechecksummed wrong map hashes, bad IDs/counts/counters/occupants/history,
fallback, failed writes and closes, readback corruption/failure, safe retries,
generation exhaustion, and independence of level state/cleared flags.

`test_storage_native` compiles the real native adapter with mock gint/BFile
headers. It asserts the SOKOBAN namespace, that all native calls remain in a
single world-switch transaction, Fugue byte-count and EOF conventions, and
bounded close-failure cleanup. It is an API contract test, **not an emulator
or hardware validation**.

Both storage test binaries pass strict compiler warnings and
UndefinedBehaviorSanitizer. The installed Apple clang AddressSanitizer runtime
hangs before test output on this host (also reproduced with an empty program),
so AddressSanitizer is **not verified**; the hung attempts were stopped.
Real fx-CG50 filesystem behavior, power-loss
timing, MENU suspension/resume, fresh-launch persistence, and remaining device
stack/heap space require the hardware retest. No promise is made that moves
after the last checkpoint survive a forced power cut.

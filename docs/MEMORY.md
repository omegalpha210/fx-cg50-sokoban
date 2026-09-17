# Memory report

Generated from the actual SuperH ELF by `tools/memory_report.py` after the native
build. Reproduce with `source tools/env.sh` then
`"$SOKOBAN_PYTHON" tools/memory_report.py`. Tool executables come from `PATH` or
`SOKOBAN_SH_CC`, `SOKOBAN_SH_SIZE`, and `SOKOBAN_SH_NM`; no private installation
path is embedded in the script. Detailed measurements are in
`build-cg/memory-report.json`.

ELF: `build-cg/sokoban`. SHA-256:
`0fb7ffc1f8856d5f7844346921eeedd2a328950c3e16aa5a33ed865134b9d205`.

## Linked sections

| `sh-elf-size` category | Bytes |
| --- | ---: |
| text (code and read-only sections) | 46,512 |
| initialized data | 464 |
| BSS | 12,400 |
| total reported sections | 59,376 |

These linked totals include pulled-in gint, C-library and compiler runtime code
and globals. They are not the `.g3a` package size, a total RAM requirement, or a
measurement of available hardware memory. Read-only map/font data are included
in text, not counted again as BSS.

## Bounded application storage

The script compiles a sizeof probe with the SH compiler and checks symbol sizes
using `sh-elf-nm`; it does not use host pointer sizes. Target pointers are
4 bytes. Linked `_app` and `_workspace` sizes agree with the probe.

| Item | Bytes | Scope |
| --- | ---: | --- |
| `SokState` active board | 92 | Dynamic occupants, counters, undo; no terrain copy |
| Five compact undo records | 5 | Included in each state; plus one count byte |
| 60 compact progress states | 5,520 | Included in progress, no full terrain copies |
| Entire `SokProgress` | 5,648 | States, per-level flags, generation, dirty flag and padding |
| Entire static `SokApp` | 5,812 | Includes progress, active state, selectors, input and hooks |
| Static save transaction workspace | 5,520 | Shared synchronous buffer, not reentrant |
| App + save workspace | 11,332 | Included in linked BSS above |

State contains up to 36 crate positions and one player position. Every position
is a 16-bit cell index; moves and pushes are 32-bit counters. Application code
does not call allocators for moves, undo, screen transitions or save transactions.
No application `malloc`, `calloc`, `realloc`, `kmalloc`, or `free` calls were found in `src/**/*.c`.

## Immutable maps and save files

| Const map data in ELF | Bytes |
| --- | ---: |
| Packed 2-bit terrain for 17,586 rectangle cells | 4,412 |
| Initial crate positions | 2,136 |
| 60 native map descriptors (16 bytes each) | 960 |
| Map-pack SHA-256 | 32 |
| Total map const symbols | 7,540 |

The current codec stores 180 bytes with no active progress. The
maximum for this specific 60-map pack, with every level in progress and five
undo records per level, is 3,336 bytes per slot: header + CRC +
60 × 2-byte ID/flags + Σ(12 + 2 × actual crate count + 5). Two full valid
slots therefore use at most 6,672 bytes of file payload, excluding
filesystem allocation overhead. The 5,520-byte
workspace is the more conservative format capacity using 36 crates for every
level. Neither size includes repeated terrain.

## Stack evidence and limits

The largest compiler-reported **single frame in this project's C sources** is
212 bytes in `sok_storage_save_io`
(`src/storage/transaction.c:133`, `static`).
The build enables `-fstack-usage` and `-Wframe-larger-than=2048`.

| Function | Frame bytes | Compiler classification | Source |
| --- | ---: | --- | --- |
| `sok_storage_save_io` | 212 | `static` | `src/storage/transaction.c:133` |
| `records` | 152 | `static` | `src/storage/codec.c:92` |
| `sok_render` | 140 | `static` | `src/ui/render.c:172` |
| `sok_validate` | 132 | `static` | `src/game/game.c:186` |
| `sok_init` | 104 | `static` | `src/game/game.c:134` |
| `text_ratio` | 76 | `static` | `src/ui/render.c:27` |
| `sok_storage_load_io` | 72 | `static` | `src/storage/transaction.c:65` |
| `main` | 64 | `static` | `src/main.c:31` |

This is not total stack peak. Nested caller/callee frames, gint/libc frames,
interrupt handling, OS world-switch state, and OS stack reservations are not
summed or measured by this report. Whole-program high-water stack and remaining
heap must be checked on an fx-CG50; no hardware safety margin is claimed.

## Display and library allocations

The installed gint fx-CG implementation was inspected in
`src/render-cg/dvram.c`, function `dvram_init`. It calls
`kmalloc(DWIDTH*DHEIGHT*2 + MARGIN*2 + 32, ...)`, with `MARGIN=32`:
396 × 224 × 2 + 64 + 32 = **177,504 bytes**. The request comprises a
177,408-byte pixel buffer and 96 bytes for alignment and rendering margins. The selected
arena depends on gint's `GINT_NO_OS_STACK` configuration. This runtime allocation
is separate from the ELF BSS total and is not proof of available heap.

The application calls `dsetvram(gint_vram, NULL)` and uses the single existing
gint VRAM. It does not allocate a second fullscreen framebuffer or keep screen
snapshots. Host renderer captures use host buffers and are not calculator RAM.
Other library/OS allocations and actual free stack/heap were not measured;
**hardware retesting remains required**.

#!/usr/bin/env python3
"""Validate the pinned pusher text maps and generate offline, const C assets.

The original bytes and metadata are retained under assets/maps/upstream.
This program never downloads content and never repairs malformed maps.
"""

from __future__ import annotations

import argparse
from collections import Counter, deque
from dataclasses import dataclass
import hashlib
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
SEPARATOR = "*" * 37
SYMBOLS = frozenset("X *.&@")
VOID, WALL, FLOOR, GOAL = range(4)


class MapError(ValueError):
    """An invalid upstream file; no automatic repair is allowed."""


@dataclass(frozen=True)
class Map:
    level: int
    width: int
    height: int
    metadata: dict[str, int]
    metadata_lines: tuple[str, ...]
    rows: tuple[str, ...]
    terrain: tuple[int, ...]
    crates: tuple[int, ...]
    player: int
    initial_on_goals: int
    padding_cells: int


def _match(pattern: str, line: str, context: str) -> re.Match[str]:
    match = re.fullmatch(pattern, line)
    if match is None:
        raise MapError(f"{context}: malformed metadata {line!r}")
    return match


def parse_maps(text: str, expected_levels: int = 60) -> list[Map]:
    """Parse exact numbered blocks. Map rows are never stripped.

    Only empty separator lines surrounding the map body are discarded.
    Width comes from metadata; short rows receive exterior space padding.
    """
    blocks = text.split(SEPARATOR)
    if len(blocks) < 3 or blocks[0].strip() or blocks[-1].strip():
        raise MapError("missing opening/closing map separator or extra text")
    maps: list[Map] = []
    for block in blocks[1:-1]:
        lines = block.splitlines()
        while lines and lines[0] == "":
            lines.pop(0)
        if len(lines) < 7:
            raise MapError("truncated map metadata/body")
        level = int(_match(r"Maze: ([0-9]+)", lines[0], "map")[1])
        context = f"level {level}"
        if level != len(maps) + 1:
            raise MapError(f"{context}: expected ID {len(maps) + 1}; duplicate, missing or reordered level")
        offsets = _match(r"File offset: ([0-9A-F]+), DS:([0-9A-F]+), table offset: ([0-9A-F]+)", lines[1], context)
        width = int(_match(r"Size X: ([0-9]+)", lines[2], context)[1])
        height = int(_match(r"Size Y: ([0-9]+)", lines[3], context)[1])
        end = int(_match(r"End: ([0-9A-F]+)", lines[4], context)[1], 16)
        length = int(_match(r"Length: ([0-9]+)", lines[5], context)[1])
        metadata = {"file_offset": int(offsets[1], 16), "ds_offset": int(offsets[2], 16),
                    "table_offset": int(offsets[3], 16), "end": end, "length": length}
        if not (0 < width < 40 and 0 < height < 25):
            raise MapError(f"{context}: dimensions {width}x{height} exceed upstream format bounds")
        if end - metadata["file_offset"] + 1 != length or length <= 0:
            raise MapError(f"{context}: End/File offset/Length mismatch")
        if metadata["file_offset"] - metadata["ds_offset"] != 0x1390 or metadata["ds_offset"] - metadata["table_offset"] != 0xFC:
            raise MapError(f"{context}: upstream DS/table offset mismatch")
        rows = lines[6:]
        while rows and rows[0] == "":
            rows.pop(0)
        while rows and rows[-1] == "":
            rows.pop()
        if len(rows) != height:
            raise MapError(f"{context}: Size Y {height}, actual map rows {len(rows)}")
        for y, row in enumerate(rows):
            if len(row) > width:
                raise MapError(f"{context}, row {y + 1}: width {len(row)} exceeds Size X {width}")
            for x, symbol in enumerate(row):
                if symbol not in SYMBOLS:
                    raise MapError(f"{context}, column {x + 1}, row {y + 1}: unknown symbol {symbol!r}")
        raw = "".join(row.ljust(width) for row in rows)
        counts = Counter(raw)
        if counts["@"] != 1:
            raise MapError(f"{context}: expected exactly one player, found {counts['@']}")
        crate_count = counts["*"] + counts["&"]
        goal_count = counts["."] + counts["&"]
        if crate_count == 0 or crate_count != goal_count:
            raise MapError(f"{context}: crates {crate_count} and goals {goal_count} must be equal and positive")
        # Flood through all non-wall cells so an actor cannot conceal an open
        # enclosure. Any meaningful cell reached from outside is an error.
        exterior: set[int] = set()
        queue: deque[int] = deque()
        for cell, symbol in enumerate(raw):
            x, y = cell % width, cell // width
            if symbol != "X" and (x in (0, width - 1) or y in (0, height - 1)):
                exterior.add(cell)
                queue.append(cell)
        while queue:
            cell = queue.popleft()
            x, y = cell % width, cell // width
            for nx, ny in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)):
                neighbor = ny * width + nx
                if 0 <= nx < width and 0 <= ny < height and neighbor not in exterior and raw[neighbor] != "X":
                    exterior.add(neighbor)
                    queue.append(neighbor)
        for cell in sorted(exterior):
            if raw[cell] != " ":
                raise MapError(f"{context}, column {cell % width + 1}, row {cell // width + 1}: {raw[cell]!r} is connected to exterior")
        padding = {y * width + x for y, row in enumerate(rows) for x in range(len(row), width)}
        if not padding.issubset(exterior):
            cell = min(padding - exterior)
            raise MapError(f"{context}, column {cell % width + 1}, row {cell // width + 1}: enclosed missing-row padding")
        terrain = tuple(VOID if i in exterior else WALL if c == "X" else GOAL if c in ".&" else FLOOR
                        for i, c in enumerate(raw))
        crates = tuple(i for i, c in enumerate(raw) if c in "*&")
        player = raw.index("@")
        if len(crates) > 255:
            raise MapError(f"{context}: crate count exceeds uint8_t representation")
        if any(terrain[cell] not in (FLOOR, GOAL) for cell in (*crates, player)):
            raise MapError(f"{context}: occupant on impassable cell")
        maps.append(Map(level, width, height, metadata, tuple(lines[:6]), tuple(rows), terrain,
                        crates, player, counts["&"], len(padding)))
    if len(maps) != expected_levels:
        raise MapError(f"expected {expected_levels} levels, found {len(maps)}")
    return maps


def pack_terrain(terrain: tuple[int, ...]) -> bytes:
    data = bytearray((len(terrain) + 3) // 4)
    for cell, value in enumerate(terrain):
        data[cell // 4] |= value << ((cell % 4) * 2)
    return bytes(data)


def verify_manifest(manifest: dict, source_dir: Path) -> None:
    """Verify all three inspected original files, including metadata evidence."""
    for entry in manifest["files"]:
        path = source_dir / entry["path"]
        content = path.read_bytes()
        if len(content) != entry["bytes"] or hashlib.sha256(content).hexdigest() != entry["sha256"]:
            raise MapError(f"pinned upstream file changed: {path.name}")


def _array(values: list[int], indent: str = "    ", width: int = 16, hex_bytes: bool = True) -> str:
    chunks = []
    for offset in range(0, len(values), width):
        chunks.append(indent + ", ".join(f"0x{x:02x}" if hex_bytes else str(x) for x in values[offset:offset + width]) + ",")
    return "\n".join(chunks)


def make_header(maps: list[Map]) -> str:
    return f'''/* Generated by tools/import_maps.py; do not edit. */
#ifndef SOKOBAN_MAPS_H
#define SOKOBAN_MAPS_H

#include <stdint.h>

#define SOK_LEVEL_COUNT {len(maps)}
#define SOK_MAX_CRATES {max(len(m.crates) for m in maps)}
#define SOK_MAX_CELLS {max(len(m.terrain) for m in maps)}
#define SOK_MAX_MAP_WIDTH {max(m.width for m in maps)}
#define SOK_MAX_MAP_HEIGHT {max(m.height for m in maps)}

enum SokTerrain {{
    SOK_VOID = 0,
    SOK_WALL = 1,
    SOK_FLOOR = 2,
    SOK_GOAL = 3
}};

/* Row-major cells; terrain uses four 2-bit cells per byte, low bits first. */
typedef struct SokMap {{
    uint8_t width;
    uint8_t height;
    uint8_t crate_count;
    uint16_t player;
    const uint8_t *terrain;
    const uint16_t *crates;
}} SokMap;

extern const SokMap sok_maps[SOK_LEVEL_COUNT];
/* SHA-256 of the exact pinned upstream plain-text map source. */
extern const uint8_t sok_map_pack_hash[32];
uint8_t sok_map_terrain(const SokMap *map, uint16_t cell);
/* IDs are 1..60. Returns NULL for an invalid ID. */
const SokMap *sok_get_map(unsigned level_id);

#endif
'''


def make_c(maps: list[Map], digest: str) -> str:
    terrain: list[int] = []
    crates: list[int] = []
    initializers = []
    for m in maps:
        initializers.append(f"    {{{m.width}, {m.height}, {len(m.crates)}, {m.player}, terrain_data + {len(terrain)}, crate_data + {len(crates)}}}, /* level {m.level} */")
        terrain.extend(pack_terrain(m.terrain))
        crates.extend(m.crates)
    return '''/* Generated by tools/import_maps.py; do not edit.
 * Map data rights are separate from code. See docs/ASSET_PROVENANCE.md.
 */
#include "maps.h"
#include <stddef.h>

static const uint8_t terrain_data[] = {
''' + _array(terrain) + '\n};\n\nstatic const uint16_t crate_data[] = {\n' + _array(crates, hex_bytes=False) + '''
};

const SokMap sok_maps[SOK_LEVEL_COUNT] = {
''' + '\n'.join(initializers) + '\n};\n\nconst uint8_t sok_map_pack_hash[32] = {\n' + _array(list(bytes.fromhex(digest))) + '''
};

uint8_t sok_map_terrain(const SokMap *map, uint16_t cell)
{
    if (map == NULL || map->terrain == NULL || cell >= (unsigned)map->width * map->height)
        return SOK_VOID;
    return (uint8_t)((map->terrain[cell / 4u] >> (2u * (cell % 4u))) & 3u);
}

const SokMap *sok_get_map(unsigned level_id)
{
    if (level_id < 1u || level_id > SOK_LEVEL_COUNT) return NULL;
    return &sok_maps[level_id - 1u];
}
'''


def statistics(maps: list[Map], manifest: dict, digest: str) -> dict:
    levels = []
    for m in maps:
        counts = Counter(m.terrain)
        levels.append({"level": m.level, "width": m.width, "height": m.height,
                       "cells": len(m.terrain), "crates": len(m.crates), "goals": counts[GOAL],
                       "player_count": 1, "player_cell": m.player,
                       "initial_crates_on_goals": m.initial_on_goals,
                       "void_cells": counts[VOID], "walls": counts[WALL],
                       "floor_cells": counts[FLOOR], "padding_cells": m.padding_cells,
                       "terrain_bytes": len(pack_terrain(m.terrain)),
                       "symbols": sorted(set("".join(m.rows))),
                       "source_row_lengths": list(map(len, m.rows)),
                       "metadata": m.metadata, "metadata_lines": list(m.metadata_lines)})
    return {"upstream_revision": manifest["revision"], "map_pack_sha256": digest,
            "level_count": len(maps), "max_width": max(m.width for m in maps),
            "max_height": max(m.height for m in maps), "max_cells": max(len(m.terrain) for m in maps),
            "max_crates": max(len(m.crates) for m in maps),
            "total_cells": sum(len(m.terrain) for m in maps),
            "terrain_bytes": sum(len(pack_terrain(m.terrain)) for m in maps),
            "initial_crate_bytes": sum(2 * len(m.crates) for m in maps),
            "symbols": sorted(set().union(*(set("".join(m.rows)) for m in maps))),
            "levels": levels}


def make_audit(stats: dict, manifest: dict) -> str:
    summary = f'''# Map audit

Generated deterministically by `python3 tools/import_maps.py`. Check without changing files:
`python3 tools/import_maps.py --check`. Both commands use local files only.

Upstream: [begoon/sokoban-maps](https://github.com/begoon/sokoban-maps/tree/{manifest['revision']}).
Pinned revision: `{manifest['revision']}`.
Plain text SHA-256 / map-pack identifier: `{stats['map_pack_sha256']}`.
The manifest in `assets/maps/` pins exact source URLs and SHA-256 values. Downloaded
text, README and extractor are retained only in local builds and excluded from the
public snapshot. No DOS executable was downloaded or executed.

All **{stats['level_count']}** maps pass structural validation in original order (IDs 1–60).
Maximum width **{stats['max_width']}**, height **{stats['max_height']}**, board cells **{stats['max_cells']}**,
crates/goals **{stats['max_crates']}**. Level 59 reaches all four maxima (29 × 20; 36 crates).
There are {stats['total_cells']} total rectangle cells. Packed terrain occupies
**{stats['terrain_bytes']} bytes**, initial crate coordinates **{stats['initial_crate_bytes']} bytes**,
plus 60 read-only map descriptors and the 32-byte pack identifier.

The original decoder explicitly maps `X` to wall, space to empty space, `*` to crate,
`.` to goal, and `&` to crate already on a goal. Its epilogue adds `@` for the player.
These are exactly the six symbols present in the source. `&` contributes once to
both crate count and goal count; its terrain remains GOAL independently of occupancy.

The importer preserves leading spaces, level order, dimensions, and every original
metadata line. Metadata width is authoritative: short rows are padded to that width,
and existing trailing spaces are retained. Rows wider than metadata and mismatched
height are rejected. Boundary-connected non-wall cells are flood-filled; all must
be plain spaces and become VOID. Missing-row padding must belong to that exterior.
Enclosed spaces become FLOOR. No level is cropped or otherwise repaired.

Validation rejects reordered/duplicate/missing IDs, malformed metadata, invalid
dimensions, inconsistent offsets/length, unknown symbols, player count other than
one, unequal or zero crate/goal counts, and occupants/goals exposed to the exterior.
Generated terrain is two bits per cell, four row-major cells per byte, low bits first.
Builds consume locally generated const data; no runtime map text parser or network access
is required. Public source users explicitly fetch the pinned inputs once before import. The pack identifier hashes the unchanged source bytes, including metadata.

BASIC (1–15), INTERMEDIATE (16–30), ADVANCED (31–45), MASTER (46–60) are
user-requested groups. No claim is made that upstream orders these maps by difficulty.
Structural validation is **not** a claim that all 60 maps were solved; no solver was run.
Map redistribution permission remains unconfirmed; see `ASSET_PROVENANCE.md`.

Coordinates in errors are one-based columns/rows of the original metadata rectangle.
The machine-readable full audit is `assets/maps/statistics.json`; it includes source
row lengths, symbols, offsets, original metadata, player position and cell counts.

| Level | Width | Height | Crates | Goals | Initially on goal | VOID | Packed terrain bytes |
|---:|---:|---:|---:|---:|---:|---:|---:|
'''
    return summary + ''.join(f"| {m['level']} | {m['width']} | {m['height']} | {m['crates']} | {m['goals']} | {m['initial_crates_on_goals']} | {m['void_cells']} | {m['terrain_bytes']} |\n" for m in stats['levels'])


def generated_outputs(source: Path, manifest_path: Path) -> dict[str, str]:
    manifest = json.loads(manifest_path.read_text())
    verify_manifest(manifest, source.parent)
    raw = source.read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    maps = parse_maps(raw.decode("ascii"))
    stats = statistics(maps, manifest, digest)
    return {"include/maps.h": make_header(maps), "src/maps/generated_maps.c": make_c(maps, digest),
            "assets/maps/statistics.json": json.dumps(stats, indent=2) + "\n",
            "docs/MAPS_AUDIT.md": make_audit(stats, manifest)}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="validate pinned originals and compare generated files without writing")
    args = parser.parse_args()
    try:
        outputs = generated_outputs(ROOT / "assets/maps/upstream/sokoban-maps-60-plain.txt", ROOT / "assets/maps/manifest.json")
        stale = []
        for name, content in outputs.items():
            path = ROOT / name
            if args.check:
                if not path.exists() or path.read_text() != content:
                    stale.append(name)
            else:
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(content)
        if stale:
            raise MapError("generated files are stale: " + ", ".join(stale))
    except (MapError, OSError, UnicodeError, KeyError, json.JSONDecodeError) as error:
        print(f"map import failed: {error}", file=sys.stderr)
        return 1
    print("60 pinned maps valid; generated assets " + ("match" if args.check else "written"))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

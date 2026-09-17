#!/usr/bin/env python3
"""Measure the SH build without running calculator code or using host sizeof.

Run after tools/build.sh, with sh-elf tools on PATH (source tools/env.sh).
Optional SOKOBAN_SH_CC, SOKOBAN_SH_SIZE and SOKOBAN_SH_NM override executables.
Generated probe/object/JSON remain in the project's build directory.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess


ROOT = Path(__file__).resolve().parents[1]


def run(command):
    return subprocess.check_output(command, text=True, cwd=ROOT)


def tool(variable, default):
    return shlex.split(os.environ.get(variable, default))


def symbols(nm, path):
    result = {}
    for line in run(nm + ["-S", "--size-sort", str(path)]).splitlines():
        match = re.fullmatch(r"[0-9a-fA-F]+\s+([0-9a-fA-F]+)\s+(\w)\s+(\S+)", line)
        if match:
            result[match[3].removeprefix("_")] = {
                "bytes": int(match[1], 16), "section_type": match[2]
            }
    return result


def relative(path):
    try:
        return str(path.resolve().relative_to(ROOT))
    except ValueError:
        return path.name


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build-cg")
    parser.add_argument("--output", type=Path, default=ROOT / "docs/MEMORY.md")
    args = parser.parse_args()
    build = args.build_dir.resolve()
    try:
        build.relative_to(ROOT)
        args.output.resolve().relative_to(ROOT)
    except ValueError:
        parser.error("build directory and report must remain inside this repository")
    elf = build / "sokoban"
    if not elf.is_file():
        parser.error("build-cg/sokoban is missing; complete the SH build first")
    cc = tool("SOKOBAN_SH_CC", "sh-elf-gcc")
    size_tool = tool("SOKOBAN_SH_SIZE", "sh-elf-size")
    nm = tool("SOKOBAN_SH_NM", "sh-elf-nm")
    sizes = run(size_tool + [str(elf)]).splitlines()[1].split()
    sections = dict(zip(("text", "data", "bss"), map(int, sizes[:3])))
    linked = symbols(nm, elf)

    # Array lengths become symbol sizes in a cross-compiled object. No target
    # executable is run, and host pointer width cannot affect the result.
    expressions = {
        "state": "sizeof(SokState)",
        "undo_records": "sizeof(((SokState *)0)->undo)",
        "progress": "sizeof(SokProgress)",
        "progress_states": "sizeof(((SokProgress *)0)->levels)",
        "app": "sizeof(SokApp)",
        "map_descriptor": "sizeof(SokMap)",
        "map_descriptors": "sizeof(sok_maps)",
        "save_workspace_capacity": "SOK_SAVE_MAX_SIZE",
        "save_header": "SOK_SAVE_HEADER_SIZE",
        "undo_limit": "SOK_UNDO_LIMIT",
        "pointer": "sizeof(void *)",
    }
    probe = build / "memory-probe.c"
    obj = build / "memory-probe.o"
    probe.write_text('#include "app.h"\n' + "".join(
        f"const unsigned char sok_size_{name}[{expression}] = {{0}};\n"
        for name, expression in expressions.items()
    ))
    subprocess.run(cc + ["-std=c11", "-Wall", "-Wextra", "-Werror", "-Os",
        "-ffreestanding", "-m4-nofpu", "-mb", "-I", str(ROOT / "include"),
        "-c", str(probe), "-o", str(obj)], check=True, cwd=ROOT)
    measured = symbols(nm, obj)
    native = {name: measured[f"sok_size_{name}"]["bytes"] for name in expressions}
    if native["app"] != linked["app"]["bytes"]:
        raise RuntimeError("SokApp changed since ELF build; rebuild before reporting")
    if native["save_workspace_capacity"] != linked["workspace"]["bytes"]:
        raise RuntimeError("save workspace changed since ELF build; rebuild first")

    frames = []
    for path in sorted(build.rglob("*.su")):
        for line in path.read_text().splitlines():
            fields = line.split("\t")
            if len(fields) != 3:
                continue
            source, line_number, _column, function = fields[0].rsplit(":", 3)
            source_path = Path(source)
            try:
                source_path.resolve().relative_to(ROOT / "src")
            except ValueError:
                continue
            frames.append({"function": function, "bytes": int(fields[1]),
                "classification": fields[2], "source": relative(source_path),
                "line": int(line_number)})
    if not frames:
        raise RuntimeError("no own-source .su records found; enable -fstack-usage")
    frames.sort(key=lambda frame: (-frame["bytes"], frame["function"]))
    maximum = frames[0]
    audit = json.loads((ROOT / "assets/maps/statistics.json").read_text())
    map_bytes = {name: linked[name]["bytes"] for name in
        ("terrain_data", "crate_data", "sok_maps", "sok_map_pack_hash")}
    if map_bytes["terrain_data"] != audit["terrain_bytes"] or \
            map_bytes["crate_data"] != audit["initial_crate_bytes"]:
        raise RuntimeError("ELF map sizes disagree with importer audit")
    # Codec v1 always stores ID+flags; in-progress records add player/counts/
    # counters (12 bytes), each crate's uint16 position, and 0..5 undo bytes.
    minimum_save = native["save_header"] + 4 + 2 * audit["level_count"]
    maximum_save = minimum_save + sum(
        12 + 2 * level["crates"] + native["undo_limit"]
        for level in audit["levels"])
    app_source = "\n".join(path.read_text() for path in (ROOT / "src").rglob("*.c"))
    allocation_calls = sorted(set(re.findall(
        r"\b((?:k|gint_)?malloc|calloc|realloc|free)\s*\(", app_source)))
    report = {
        "elf": relative(elf),
        "elf_sha256": hashlib.sha256(elf.read_bytes()).hexdigest(),
        "sections_bytes": sections,
        "native_sizes_bytes": native,
        "linked_app_bss_bytes": linked["app"]["bytes"],
        "linked_workspace_bss_bytes": linked["workspace"]["bytes"],
        "map_const_bytes": map_bytes,
        "map_const_total_bytes": sum(map_bytes.values()),
        "save_minimum_bytes": minimum_save,
        "save_maximum_current_pack_bytes": maximum_save,
        "application_allocation_calls": allocation_calls,
        "own_source_frames": frames,
        "gint_vram_allocation_bytes": 396 * 224 * 2 + 32 * 2 + 32,
    }
    (build / "memory-report.json").write_text(json.dumps(report, indent=2) + "\n")
    top_frames = "\n".join(
        f"| `{frame['function']}` | {frame['bytes']:,} | `{frame['classification']}` | `{frame['source']}:{frame['line']}` |"
        for frame in frames[:8])
    allocation_note = (
        "No application `malloc`, `calloc`, `realloc`, `kmalloc`, or `free` calls were found in `src/**/*.c`."
        if not allocation_calls else
        "Application allocator calls found: " + ", ".join(f"`{name}`" for name in allocation_calls) + ".")
    document = f"""# Memory report

Generated from the actual SuperH ELF by `tools/memory_report.py` after the native
build. Reproduce with `source tools/env.sh` then
`\"$SOKOBAN_PYTHON\" tools/memory_report.py`. Tool executables come from `PATH` or
`SOKOBAN_SH_CC`, `SOKOBAN_SH_SIZE`, and `SOKOBAN_SH_NM`; no private installation
path is embedded in the script. Detailed measurements are in
`{relative(build / 'memory-report.json')}`.

ELF: `{relative(elf)}`. SHA-256:
`{report['elf_sha256']}`.

## Linked sections

| `sh-elf-size` category | Bytes |
| --- | ---: |
| text (code and read-only sections) | {sections['text']:,} |
| initialized data | {sections['data']:,} |
| BSS | {sections['bss']:,} |
| total reported sections | {sum(sections.values()):,} |

These linked totals include pulled-in gint, C-library and compiler runtime code
and globals. They are not the `.g3a` package size, a total RAM requirement, or a
measurement of available hardware memory. Read-only map/font data are included
in text, not counted again as BSS.

## Bounded application storage

The script compiles a sizeof probe with the SH compiler and checks symbol sizes
using `sh-elf-nm`; it does not use host pointer sizes. Target pointers are
{native['pointer']} bytes. Linked `_app` and `_workspace` sizes agree with the probe.

| Item | Bytes | Scope |
| --- | ---: | --- |
| `SokState` active board | {native['state']:,} | Dynamic occupants, counters, undo; no terrain copy |
| Five compact undo records | {native['undo_records']:,} | Included in each state; plus one count byte |
| 60 compact progress states | {native['progress_states']:,} | Included in progress, no full terrain copies |
| Entire `SokProgress` | {native['progress']:,} | States, per-level flags, generation, dirty flag and padding |
| Entire static `SokApp` | {native['app']:,} | Includes progress, active state, selectors, input and hooks |
| Static save transaction workspace | {native['save_workspace_capacity']:,} | Shared synchronous buffer, not reentrant |
| App + save workspace | {native['app'] + native['save_workspace_capacity']:,} | Included in linked BSS above |

State contains up to 36 crate positions and one player position. Every position
is a 16-bit cell index; moves and pushes are 32-bit counters. Application code
does not call allocators for moves, undo, screen transitions or save transactions.
{allocation_note}

## Immutable maps and save files

| Const map data in ELF | Bytes |
| --- | ---: |
| Packed 2-bit terrain for 17,586 rectangle cells | {map_bytes['terrain_data']:,} |
| Initial crate positions | {map_bytes['crate_data']:,} |
| 60 native map descriptors ({native['map_descriptor']} bytes each) | {map_bytes['sok_maps']:,} |
| Map-pack SHA-256 | {map_bytes['sok_map_pack_hash']:,} |
| Total map const symbols | {sum(map_bytes.values()):,} |

The current codec stores {minimum_save:,} bytes with no active progress. The
maximum for this specific 60-map pack, with every level in progress and five
undo records per level, is {maximum_save:,} bytes per slot: header + CRC +
60 × 2-byte ID/flags + Σ(12 + 2 × actual crate count + 5). Two full valid
slots therefore use at most {maximum_save * 2:,} bytes of file payload, excluding
filesystem allocation overhead. The {native['save_workspace_capacity']:,}-byte
workspace is the more conservative format capacity using 36 crates for every
level. Neither size includes repeated terrain.

## Stack evidence and limits

The largest compiler-reported **single frame in this project's C sources** is
{maximum['bytes']:,} bytes in `{maximum['function']}`
(`{maximum['source']}:{maximum['line']}`, `{maximum['classification']}`).
The build enables `-fstack-usage` and `-Wframe-larger-than=2048`.

| Function | Frame bytes | Compiler classification | Source |
| --- | ---: | --- | --- |
{top_frames}

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
"""
    args.output.write_text(document)
    print(f"memory: text={sections['text']} data={sections['data']} bss={sections['bss']}; "
        f"SokApp={native['app']} SokState={native['state']} "
        f"max-own-frame={maximum['bytes']} ({maximum['function']}); "
        f"max-save={maximum_save} bytes")


if __name__ == "__main__":
    main()

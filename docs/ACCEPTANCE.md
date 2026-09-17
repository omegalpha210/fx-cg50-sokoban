# Acceptance — v0.1.0-beta.2

This milestone redesigns only the CASIO Main Menu icon and updates its generator,
geometry tests, previews, documentation and package/release version. The engine,
60 maps, movement/UNDO, saves, SHIFT+AC/ON, menus, gameplay renderer and MENU/EXIT
lifecycle are unchanged from beta.1. Both native icon records use the new artwork.

The eligible prerelease remains **source-only**. Original code/icons are MIT;
separately identified dependencies retain their notices. Upstream-map redistribution
is still unconfirmed. No raw/generated map pack, bundled `.g3a`, its release
checksum, or upstream-map screenshot is published. See [publication audit](PUBLICATION_AUDIT.md).

## Baseline and unchanged runtime

The clean local baseline was `main` at `bf88d8bf796e59aed21cc5569b576c1acc6da7a6`;
the verified public main/tag beta.1 was `b817f1057d8c591aed739dcb8d64031da8d08b32`.
No newer public version or conflicting beta.2 tag existed at the initial check.
Existing public history is retained, with a normal descendant update and new tag.
Local development history remains private because it includes retained map data.

`src/`, `include/` and map assets have no diff from the local baseline. The clean
native `sokoban.bin` remains byte-identical: 46,976 bytes, SHA-256
`7e92e0d3c4927141ece4172a5e91ce2030451c8ef9b109f0f86358498fbcbfed`.
Only G3A icon/version metadata and corresponding checksums change in the package.
App name `SOKOBAN`, internal ID `@SOKOBAN`, save names and format version 1 remain.
DIFF EQ was inspected read-only; its source/output/history remain independent.
The existing toolchain was reused without reinstalling or upgrading it.

## Icon acceptance

The supplied 92×64 image visually matches the old asset. Old wall blocks were
11×11px, while the crate was 17×17px. Its lower edge y46 left 17 clear rows, and
hardware feedback still found it close to the OS label. The new original scene
uses an 8×4 grid of 10×10 cells at `(6,2)`, with player → crate → target in adjacent
cells. It is redrawn geometry, not a translation or downscaled previous bitmap.

| Measure | beta.1 | beta.2 |
|---|---:|---:|
| Canvas, both opaque RGB variants | 92×64 | 92×64 |
| Wall outer footprint | 11×11 | 10×10 |
| Crate outer footprint | 17×17 | 10×10 |
| Crate orange inner fill | 13×13 | 8×8 |
| Inclusive artwork bounds | (8,1)..(83,46) | (6,2)..(85,41) |
| Top margin | 1px | 2px |
| Bottom margin | 17px | 22px |
| Normal PNG bytes | 658 | 478 |
| Selected PNG bytes | 652 | 480 |
| Native RGB565 bytes per variant | 11,776 | 11,776 |

Wall/crate footprints are measured as fully occupied equal-size cells. The black
player and small hollow goal fit within one cell each. Flat colors and integer
coordinates avoid antialiasing. No text, external sprite, commercial Sokoban art
or CASIO icon artwork is used. Both variants have identical 80×40 artwork;
selected changes only the surrounding background. There is no enclosing thick
rounded frame. Bottom rows 42..63 are plain background.

The direct PNG → fxgxa RGB565 pipeline is preserved; icons are packaged metadata,
not runtime graphics or a new icon cache. The separate CASIO name fields still
supply `SOKOBAN`. [ICON_AUDIT.md](ICON_AUDIT.md) records sources, precise grid,
DIFF EQ numeric reference, hashes, normal/selected comparison, 8× nearest-neighbor
previews and a clearly illustrative label-safe-area mock. The optional actual
DIFF EQ side-by-side is local-only, outside the public snapshot.

## Automated evidence

Native and host build directories were empty for the clean regression run.

| Check | Result |
|---|---|
| Python | **39 passed**: original 24, icon 8, layout 3, publication safety 4. |
| Icon geometry | Deterministic regeneration; 92×64 RGB; flat palette; safe bounds; every cell on the same 10px grid; equal actual wall/crate footprints; player/goal placement; selected visibility; no text drawing. |
| C/UBSan | **7/7 suites passed**: engine, storage, workflow, renderer, native BFile, power and public renderer. |
| Engine | 359,247 assertions; 76,275 legal randomized moves/undo and 1,500 pushes across 60 maps. |
| Workflow | 895 assertions, retaining Main/all-group row-major navigation and lifecycle behavior. |
| Native storage/power | 297 BFile calls within 9 complete mocked OS transactions; save-before-OFF, failure, chords and one-shot behavior passed. |
| SH | Clean strict compile/link; **0 compiler warnings**. |
| Package | **15 checks passed**; normal and selected RGB565 records match the new PNGs byte-for-byte. |
| Maps/save | Unchanged pinned pack, all 60 state/history round-trips and prior corruption/fallback coverage. |
| Renderer | 87 local full-pack captures and both contact sheets regenerated identically; 5 public original-fixture views remain unchanged. |
| ASan | Not verified: this host runtime stalls before main, including an empty program. UBSan passes. |

The same full checks also passed in the exact public candidate, hydrated locally
with pinned inputs and rebuilt from empty native/host build directories. Icon
regeneration is byte-identical; its native payload and full package match the
development build. Generated maps and build outputs stay ignored. Public source
ZIP contents are verified against the tag after publication; binary upload/download
verification is not applicable while map-bundled binary distribution is withheld.

## Native size and local package

| Measurement, bytes | beta.1 | beta.2 |
|---|---:|---:|
| ELF text | 46,512 | 46,512 |
| data | 464 | 464 |
| BSS | 12,400 | 12,400 |
| Largest project single stack frame | 212 | 212 |
| Active state | 92 | 92 |
| Entire app | 5,812 | 5,812 |
| Entire progress | 5,648 | 5,648 |
| Save workspace | 5,520 | 5,520 |
| Maximum save per slot | 3,336 | 3,336 |
| Local `.g3a` | 75,652 | 75,652 |

No new runtime memory, framebuffer, board copy or icon cache is introduced.
Single-frame size is not total stack usage or free heap. See [MEMORY.md](MEMORY.md).

The final local `dist/SOKOBAN.g3a` is 75,652 bytes, SHA-256
`1b894f0ffe6c18ea40df0cda0c084dcd44c1f72818a6065e42d2f3756277f731`.
`dist/SHA256SUMS.txt` agrees. `SOURCE_DATE_EPOCH=1789660800` fixes header time for
candidate/development comparison. Numeric CASIO version is `00.01.0002`.
This hash identifies local validation output, not a public binary release asset.

## Remaining physical checks

The owner reported prior basic gameplay working. No new physical icon acceptance
is claimed. **HARDWARE TEST REQUIRED:** 11 priority icon checks cover equal apparent
wall/crate size, player/goal recognition, push-puzzle identity, actual OS-label
gap, no top clipping, both selection states, relative size among Main Menu apps,
and separation comparable to DIFF EQ. Remaining beta.1 physical navigation,
LCD, OFF/ON, persistence and memory checks are retained in
[HARDWARE_RETEST.md](HARDWARE_RETEST.md). Host mock label placement is illustrative,
not an OS screenshot or proof of a sufficient real label gap.

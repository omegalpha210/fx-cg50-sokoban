# CASIO Main Menu icon redesign — v0.1.0-beta.2

The user-supplied 92×64 screenshot was visually compared with the existing PNG. The beta.1 generator used 11×11 wall blocks and a 17×17 crate, matching the reported oversize. Its artwork ended at y46 and hardware feedback still found it close to the OS label. This milestone redraws the scene on a common grid; it is not another translation of that bitmap.

## Source and exact geometry

- Old source: `tools/make_icons.py` at public tag `v0.1.0-beta.1`; original PNG bytes are preserved as `docs/public-captures/icon-uns-before.png` and `icon-sel-before.png`.
- New source: deterministic `tools/make_icons.py`; outputs `assets/icon-uns.png` and `assets/icon-sel.png`. Both are opaque RGB, **92×64**. No external sprites, commercial Sokoban art, CASIO art, text or fonts are used.
- Shared integer grid: **8 columns × 4 rows, 10×10 px per cell**, origin `(6,2)`. Wall and crate outer footprints both fill one **10×10** cell; orange inner fill is **8×8**, inset by one pixel.
- The black player fits in a 7×8 footprint and the dark-gold hollow target in 5×5, each inside one cell. Player, crate, target occupy consecutive cells `(2,2)`, `(3,2)`, `(4,2)` (zero-based).
- Walls use flat dark green outlines/green fill, floor pale mint, crate orange, player black. A partial warehouse wall layout leaves open floor and avoids a heavy enclosing border. The icon scene is project-authored, not an upstream puzzle.
- Native-resolution drawing uses integer rectangles/lines/points with no antialiasing or image scaling. The selected variant changes only the surrounding background to dark green; the entire 80×40 artwork is identical, retaining player/crate/target visibility.
- New bounds are **(6,2)..(85,41)**. Top margin is **2px**, bottom margin **22px** (previously 17px). Rows 42..63 contain only the variant background. The lower edge is below the conservative limit y43.

## Actual packaging pipeline

`CMakeLists.txt::generate_g3a` passes both PNGs directly to fxgxa through `ICONS`; there is no fxconv icon transformation. Installed `GenerateG3A.cmake` maps these to `--icon-uns` and `--icon-sel`. `tools/package.sh` repeats that packaging deterministically when SOURCE_DATE_EPOCH is set. fxgxa stores two 92×64×2 RGB565 records (11,776 bytes each) at offsets 0x1000 and 0x4000. Their exact bytes are checked against the PNGs. Icons are G3A metadata, not runtime graphics/cache.

App name `SOKOBAN`, internal ID `@SOKOBAN`, and save namespace remain unchanged. The OS name comes from separate G3A name/language-label metadata, not icon text. CASIO package version advances to `00.01.0002`; the game/runtime payload is unchanged.

## Measured assets and placement reference

DIFF EQ was inspected read-only, including its 92×64 normal/selected PNGs, 4× supersampled generator and CMake declarations. Its generator explicitly leaves the lower title strip free. All faint antialiasing pixels count in the bounds below. It is a placement reference only; none of its art is incorporated into SOKOBAN. This redesign deliberately uses a larger lower margin than that reference because of the reported physical overlap.

| Variant / stage | Inclusive bounds | Top px | Bottom px | PNG bytes |
|---|---|---:|---:|---:|
| uns / before | (8,1)..(83,46) | 1 | 17 | 658 |
| uns / after | (6,2)..(85,41) | 2 | 22 | 478 |
| uns / diffeq_reference | (3,3)..(87,49) | 3 | 14 | 3698 |
| sel / before | (8,1)..(83,46) | 1 | 17 | 652 |
| sel / after | (6,2)..(85,41) | 2 | 22 | 480 |
| sel / diffeq_reference | (4,2)..(87,49) | 2 | 14 | 4425 |

## Previews and reproduction

- [Current beta.1 versus new beta.2, both variants](public-captures/icon-before-after.png).
- [New unselected, 8× nearest-neighbor](public-captures/icon-uns-8x.png); [selected, 8×](public-captures/icon-sel-8x.png).
- [Illustrative label-safe-area mock](public-captures/icon-label-safe-mock.png). The tinted reserve and sample label are explanatory overlays, absent from both icon assets. The sample y52 label is not a measured or emulated CASIO label position.
- An optional actual DIFF EQ/SOKOBAN side-by-side is generated locally as `docs/captures/icon-diffeq-placement.png`. Reference artwork stays out of the public snapshot; the numeric evidence above is public.

Run `python3 tools/make_icons.py`, then `python3 tools/icon_audit.py`. Optional `--reference-dir "$SOKOBAN_DIFFEQ_REFERENCE"` refreshes read-only reference measurements and the local comparison. `--check` verifies committed numeric/Markdown evidence without needing that checkout. SHA-256 and PNG color counts are in `ICON_AUDIT.json`.

Eight icon tests cover deterministic PNGs, opaque flat palette, native canvas, clear top/bottom margins, equal measured wall/crate footprints, all-cell grid alignment, player/crate/goal placement, preserved selected artwork, absence of text drawing, audit dimensions and both native RGB565 records. No runtime game/UI/input/storage source is changed.

**HARDWARE TEST REQUIRED:** actual OS-label gap, no top clipping, natural wall/crate scale, identifiable player/goal/push-puzzle scene, selected/unselected contrast, relative size beside other apps and separation comparable to DIFF EQ. Host geometry and mock labels cannot establish physical OS acceptance. See the priority icon checks in [HARDWARE_RETEST.md](HARDWARE_RETEST.md).

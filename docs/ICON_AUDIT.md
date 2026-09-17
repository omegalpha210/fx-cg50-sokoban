# CASIO Main Menu icon audit

The icon contains original drawn wall/crate/player geometry and **no baked-in SOKOBAN text**. Both variants are opaque RGB PNGs, 92×64. The bottom strip is the image background (white when unselected, dark green when selected), not transparency.

`CMakeLists.txt` passes `assets/icon-uns.png` and `assets/icon-sel.png` to `generate_g3a`, which invokes fxgxa to encode the two native RGB565 icon records. The installed `GenerateG3A.cmake` was checked directly: `ICONS` becomes `--icon-uns`/`--icon-sel` and `NAME` becomes `-n`. Installed fxgxa `edit.c:edit_name` writes the header name and language-label fields; `edit_g3a_icon` copies 92×64×2 bytes per variant. Icons are not loaded or cached by the app at runtime. `NAME "SOKOBAN"` supplies the OS app-name metadata, and the internal identity remains `@SOKOBAN`; the CASIO menu draws its app label separately. The project icon generator calls no text/font drawing routine.

The reported hardware overlap is therefore a placement issue: the lower edge of the graphic formerly reached y49, leaving 14 clear rows. The source geometry is now translated upward by 3 px, preserving its size, colors, all artwork pixels, and a 1 px top margin. The lower edge is y46 and the bottom margin is 17 px. This is the greatest whole-pixel upward translation that preserves a nonzero top margin; further movement would require reducing the graphic or touching the top edge.

The DIFF EQ icon generator explicitly keeps the bottom title strip free for the OS label. Its actual antialiased non-background bounds were measured read-only; only the placement measurements below are retained. No DIFF EQ artwork was copied. Its faint antialiasing fringes are included in the bounds, so top margins need not equal the source geometric endpoints.

| Icon | Canvas | Inclusive artwork bounds | Top margin | Bottom margin |
|---|---|---|---:|---:|
| uns: before | 92×64 | (8, 4)..(83, 49) | 4 | 14 |
| uns: after | 92×64 | (8, 1)..(83, 46) | 1 | 17 |
| uns: diffeq_reference | 92×64 | (3, 3)..(87, 49) | 3 | 14 |
| sel: before | 92×64 | (8, 4)..(83, 49) | 4 | 14 |
| sel: after | 92×64 | (8, 1)..(83, 46) | 1 | 17 |
| sel: diffeq_reference | 92×64 | (4, 2)..(87, 49) | 2 | 14 |

[Actual before/after preview](public-captures/icon-before-after.png) uses only SOKOBAN pixels at 3× nearest-neighbor enlargement. The original PNG snapshots were preserved before regeneration. Numeric results and input hashes are in `ICON_AUDIT.json`.

Reproduce with `python3 tools/make_icons.py` then `python3 tools/icon_audit.py`. Optional `--reference-dir "$SOKOBAN_DIFFEQ_REFERENCE"` refreshes reference measurements without changing that repository. `python3 tools/icon_audit.py --check` needs no DIFF EQ checkout. Tests verify every artwork pixel survives the translation, both RGB565 package icon records match the new PNGs, and the new safe margins are blank.

**HARDWARE RETEST REQUIRED:** actual CASIO Main Menu icon/label separation, selected and unselected contrast, and top-edge visibility. The 3 px change is measurable; host preview alone cannot establish whether the physical OS label gap is sufficient.

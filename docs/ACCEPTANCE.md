# Acceptance — v0.1.0-beta.1

Navigation, gameplay layout, icon placement and safe power-off changes are
implemented. The eligible GitHub prerelease is **source-only**: original code,
permitted font/utility notices and original illustration fixtures. Upstream map
redistribution remains unconfirmed, so no bundled `.g3a`, map pack or upstream
map screenshot is a public release asset. See [publication audit](PUBLICATION_AUDIT.md).

The owner reports prior basic play working on an fx-CG50 and an icon/OS-label
spacing problem. New behavior is host/mock tested and compiled; actual revised
LCD layout, OS label gap and physical OFF/ON remain **HARDWARE RETEST REQUIRED**.

## Preserved baseline and scope

Development began from `main` at `20d36ce6ebdb9c45da2614b6cb4febb1834bee81`,
with a clean working tree and no existing tags or remotes. Existing host suites
passed before changes. DIFF EQ was inspected read-only for getkey power handling,
MENU/Fugue, icons and clean publication policy. Its source/history/output are
unchanged; no existing toolchain was reinstalled or upgraded.

Movement, push/goal rules, five-step UNDO, completion, two-slot transaction,
map topology/hash and save-format version1 are unchanged. The public source uses
an independent clean history because the prior local commit includes maps and
machine-specific artifacts. Its original ancestry is not pushed.

## Navigation

A single small selector helper now implements RIGHT as next numeric item and
LEFT as previous, wrapping the entire2×2 or5×3 grid. Row ends continue into the
next/previous row. UP/DOWN preserve the column and wrap vertically. Numeric Main
shortcuts and EXE/F6 OPEN remain. Gameplay LEVEL-/+ remains separately bounded
within1..60, including group boundaries.

The workflow suite covers forward/reverse Main cycles, vertical columns and
all four level groups'5→6,10→11,15→1 equivalents and reverse boundaries, plus
existing INIT/completion/save/MENU/HOLD regressions.

## Gameplay layout and icon

The gameplay-only24px title is gone; no group title is relocated elsewhere.
Main/level headers remain. HUD shifts up24px with the existing font. The board
viewport expands from `(124,28) 268×172` to `(124,4) 268×196`, remains square-tiled
and centered on both axes, and stops before the unchanged softkey strip.
Gameplay INIT/win panels center at y102 in the new usable rectangle.

|60-map measurement|Before|After|
|---|---:|---:|
|Minimum tile|8px|9px|
|Maximum tile|17px|19px|
|Strict width bottleneck|9|14|
|Strict height bottleneck|48|36|
|Equal integer capacities|3|10|

**48 levels improve;12 remain unchanged; maximum gain+2px.** Improved IDs:
2,5,6,7,8,9,10,11,12,14,15,16,17,19,20,21,22,24,25,26,28,30,31,32,33,34,35,
36,37,38,39,40,41,42,43,44,46,47,48,49,50,51,53,54,55,56,58,59.
8→9 IDs:10,19,20,22,30,54,58,59. 9→10 IDs:9,24,36,39,42,44,53.
The29×20 level59 improves8→9: previously height-limited, now tied at integer9.
Full per-level geometry is in [LAYOUT_AUDIT.md](LAYOUT_AUDIT.md).

Both92×64 opaque RGB icons contain geometry and no baked-in SOKOBAN text.
The artwork bounds moved from inclusive `(8,4)..(83,49)` to `(8,1)..(83,46)`;
bottom clear margin14→17px, top1px, no resizing or clipping. DIFF EQ reference
bounds were unselected `(3,3)..(87,49)`, selected `(4,2)..(87,49)`. Only placement
was referenced. fxgxa icon pixels and separate app-name metadata were verified.
See [icon audit](ICON_AUDIT.md) and [comparison](public-captures/icon-before-after.png).

## Power-off ordering

Installed gint2.11.0 `getkey_opt()` handles fresh SHIFT+AC/ON without ALPHA via
`gint_poweroff(true)`; DIFF EQ inherits this supported path. SOKOBAN retains raw
keydev events so the app can checkpoint first. Tap/held SHIFT plus fresh AC/ON
works before every screen/modal dispatcher, including save errors. SHIFT/AC alone
and repeated HOLD do not request OFF. Modifiers are cleared and held keys gated
across transitions and ON resume.

Dirty state performs one synchronous existing two-slot checkpoint/save attempt,
including close/readback, before OFF. Clean state writes nothing. Failure keeps
dirty RAM/generation and `power_save_failed`, then proceeds with OFF without a
blocking modal. No interrupt I/O or forced mid-write power cut was added. gint
may return to the suspended execution after ON; a true new launch starts Main
and loads the existing saves. See [POWER.md](POWER.md) for exact API references.

## Automated evidence

|Check|Result|
|---|---|
|Python|**34 passed**: existing24 plus icon3, layout3 and publication safety4.|
|C/UBSan|**7/7 suites passed**: engine, storage, workflow, renderer, native BFile, power and public renderer.|
|Engine|359,247 assertions;76,275 legal randomized moves/undo and1,500 pushes across60 maps; original fixture solution.|
|Workflow|895 assertions; new horizontal/vertical navigation plus retained lifecycle tests.|
|Power|Actual `src/main.c` with installed-keycode mocks plus real input/app/codec/two-slot transaction; all screens, chord order, clean no-write, one-shot, failure and serialization-before-OFF.|
|Native storage|297 BFile calls within9 complete mocked OS world switches.|
|Save/map integrity|All60 independent states/history round-trip; truncation/checksum/occupant/undo/map-hash rejection and fallback preserved. Pinned map pack unchanged.|
|SH|Strict compile/link succeeds with **0 compiler warnings**; `-Wall -Wextra -Werror`, stack-frame threshold2048 and stack-usage output.|
|Package|**15 checks passed**; both encoded RGB565 icon records match current PNGs byte-for-byte; app name/identity unchanged.|
|Captures|**87 full-pack frames** plus full60/review contact sheets; bounds/centering/title/gutter checks. **5 public frames** link only the independently authored fixture.|
|ASan|Not verified: unchanged Apple runtime hangs before main, including an empty program. UBSan works; no reinstall attempted.|

All60 original maps pass structural checks, not a proof that all60 were solved.
Local full-map captures are excluded from public Git history. Public figures use
the actual shared renderer and are labeled as original-layout host fixtures.
Physical calculator execution is not inferred from these tests.

## Memory and local package

|Native measurement|Baseline|Updated|
|---|---:|---:|
|ELF text|46,304|46,512|
|data|464|464|
|BSS|12,384|12,400|
|Largest project single frame|212|212|
|Active state|92|92|
|Five undo bytes|5|5|
|Entire progress|5,648|5,648|
|Entire app|5,804|5,812|
|Save workspace|5,520|5,520|
|Maximum save/slot|3,336|3,336|
|Local `.g3a`|75,444|75,652|

No application heap allocation, extra runtime framebuffer, board copy or icon
cache was added. gint still owns its existing startup VRAM allocation. The212-byte
frame is not total stack peak or measured remaining heap. [MEMORY.md](MEMORY.md)
contains cross-compiled sizes and limitations.

The updated local build is `dist/SOKOBAN.g3a`,75,652 bytes, SHA-256:
`d3b6b564e951e19469c6c7f605daff3fd31f260818f3a7fd6f895adf1c46e17d`.
This identifies local validation output, **not an available release asset**.
`SOURCE_DATE_EPOCH=1789660800` fixes the package header date for source-candidate
comparison. Save magic/hash/version and `@SOKOBAN` namespace remain unchanged;
CASIO numeric package version is now `00.01.0001` for beta1.

The clean source-only candidate was independently hydrated with pinned inputs
and rebuilt from empty native/host build directories. All checks above passed
again there; its package matches the development artifact byte-for-byte and by
SHA-256. Generated maps/binaries remain ignored. The clean build also verified
automatic creation of the package output directory and TLS-verified input fetch.
Public source archive verification replaces binary release-asset
re-download verification, which is withheld with the binary itself. Actual
repository/tag/commit results are reported in the delivery record.

Follow [HARDWARE_RETEST.md](HARDWARE_RETEST.md) for all38 navigation, layout, icon,
power, persistence and resource checks. Updated real OFF/ON, physical key timing,
Fugue behavior, LCD readability and stack/heap headroom remain unverified.

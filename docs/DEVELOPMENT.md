# Development

## Toolchain and first build

Use an existing fxSDK/gint/SH compiler installation; this task reused fxSDK2.11.0,
gint2.11.0, SH GCC14.1.0, CMake4.4.3 and Apple clang17.0.0 without reinstalling or
upgrading anything. Pinned installed source revisions are in
`tools/toolchain-lock.json`. Python3 with Pillow is used by renderer/icon audits.

Put the installed toolchain on PATH, or create a gitignored local configuration:

```sh
mkdir -p .local
cat > .local/env.sh <<'CONFIG'
export SOKOBAN_SDK_ROOT="/path/to/existing/sdk"
export SOKOBAN_PYTHON="/path/to/python-with-pillow"
CONFIG
```

The SDK variable follows the existing `prefix/bin`,
`prefix/share/fxsdk/sysroot/bin`, optional `venv/bin` layout. For a different
layout, set PATH directly instead. `tools/env.sh` supports bash and zsh. CMake
and published configuration contain no personal installation paths.

The source-only public snapshot omits map inputs and embedded data. Explicitly
acquire the immutable sources for a local build:

```sh
python3 tools/fetch_maps.py
python3 tools/import_maps.py
bash tools/validate.sh
```

The fetch command verifies the three URLs, sizes and SHA-256 values in
`assets/maps/manifest.json`, never downloads the DOS executable, and is never
called implicitly during ordinary builds. It does not establish redistribution
permission. The importer preserves topology/order/symbols and generates compact
const data; there is no runtime text parser. `--check` validates without changes.

If a Python installation has no default CA bundle, point `SSL_CERT_FILE` at an
existing trusted bundle before fetching (on this macOS host, `/etc/ssl/cert.pem`).
TLS certificate verification stays enabled; do not bypass it. No toolchain or
certificate installation was needed for the validated build.

`tools/validate.sh` builds the package, checks map/layout/icon integrity, runs all
Python and C/UBSan suites, generates local/public renderer captures, and records
memory and package evidence. The individual commands remain:

```sh
bash tools/build.sh
bash tools/test.sh
bash -c 'source tools/env.sh; "$SOKOBAN_PYTHON" tools/captures.py'
bash -c 'source tools/env.sh; "$SOKOBAN_PYTHON" tools/public_captures.py'
bash -c 'source tools/env.sh; "$SOKOBAN_PYTHON" tools/memory_report.py'
```

Local `build-host/`, `build-cg/`, `.local/`, `dist/`, downloaded/generated map data,
and full-map captures are not public source assets. The normal package is
`dist/SOKOBAN.g3a`, with `dist/SHA256SUMS.txt`. Program name/internal identity
remain `SOKOBAN` / `@SOKOBAN`. The prerelease is `v0.1.0-beta.3`; the numeric CASIO
version field is `00.01.0003` (the CASIO field has no beta suffix).

`tools/package.sh` honors `SOURCE_DATE_EPOCH` for a fixed UTC header date. Set the
same value when comparing local and clean-public-source builds. Without it,
fxgxa uses the current build date, which can change the package hash even when
code is identical. No existing release tag is moved or overwritten.

## Architecture and validation

Game rules and storage-format version1 are unchanged. `src/game/` stays pure and
host-testable; `src/app.c` is the iterative screen/modal state machine; `src/ui/`
owns rendering and input gates; `src/storage/` owns byte codec, two-slot
transactions and native BFile. There are no recursive screen calls, per-move
allocations, new runtime framebuffer or icon cache.

The shared selector helper uses row-major modulo traversal for LEFT/RIGHT and
same-column vertical modulo for UP/DOWN. Play LEVEL-/+ are separate and bounded.
The gameplay-only title is removed, board viewport is268×196 at(124,4), and HUD
moves up24px. Full60 before/after measurements are in [LAYOUT_AUDIT.md](LAYOUT_AUDIT.md).
The beta.2 icon is redrawn on a 10×10 grid with equal wall/crate footprints.
Opaque 92×64 PNGs retain 22 clear lower rows; the selected state changes only
the surrounding background. Regenerate with `python3 tools/make_icons.py`, then
`python3 tools/icon_audit.py`. [ICON_AUDIT.md](ICON_AUDIT.md) records pixel bounds,
8× previews, an explicitly illustrative label mock and native RGB565 verification.
The beta.2 icon is retained in beta.3. The game rules, map pack and save format remain unchanged.

Raw `keydev_read(...,false,NULL)` retains release events and leaves MENU/OFF
under application control. The main loop checks the RTC idle policy and sleeps
between existing keyboard scanner interrupts. `src/idle.c` is host-testable;
`src/system_power.c` reads settings and sets transient brightness in the OS world.
See [POWER.md](POWER.md) for syscall sources and the CG50 level-0 hardware check. Arrows repeat after500ms,
then every125ms, with one deterministic direction. Control keys require fresh
presses; late HOLD after UP is rejected. The input layer recognizes tapped/held
SHIFT plus fresh AC/ON and excludes ALPHA. See [POWER.md](POWER.md) for actual
installed gint/DIFF EQ source references and finite failure semantics.

Storage remains synchronous on the main thread inside a complete OS world
switch. `gint_poweroff(true)` runs only after the attempted transaction returns;
OFF does not wait in an error modal; a failed checkpoint displays the retry
dialog after ON. MENU also works from a save-error dialog.
ON may resume the suspended execution; a true fresh main starts at Main and loads
verified saves. No claim of real hardware power validation is made.

Host C uses strict warnings, assertions enabled in optimized builds and
non-recovering UBSan. SH uses `-Wall -Wextra -Werror -Wframe-larger-than=2048
-Os -g -fstack-usage`. Optional `-DSOK_ASAN=ON` is available, but this machine's
Apple ASan runtime hangs before main, including an empty program. ASan is not
verified; UBSan passes. Single-frame reports do not measure total stack peak.

## Clean publication

The owner's explicit request authorizes a public source repository and prerelease.
Development history contains locally retained map content and must remain local.
`tools/public_snapshot.py NEW_DIRECTORY` exports an allowlisted clean candidate;
it never rewrites an existing directory or mutates a remote. It excludes raw and
generated maps, bundled binaries, full-map captures, private logs/configuration,
manuals/toolchains and Git history. `--check-tracked` audits a candidate's tracked
paths and contents for those exclusions, private paths and credential patterns.

The public screenshots link the same renderer to `tests/public/fixture.c`, an
independently authored illustration map. This fixture is not a substitute for the
product's real60 maps and is never linked into the calculator executable.
[PUBLICATION_AUDIT.md](PUBLICATION_AUDIT.md) records current rights evidence.

Before publishing, explicitly fetch/import pinned inputs inside the candidate,
run its complete validation from a clean build, verify icon bytes/package and
compare the resulting local package with the development build. Check generated
maps match and remain ignored; do not add them to Git. Publish only the eligible
clean source tree under MIT plus third-party notices. With map rights unresolved,
the prerelease has no `.g3a` or checksum release assets; verify GitHub source
archive contents against the tagged commit instead. Report withheld binary
upload/download verification accurately as not applicable.

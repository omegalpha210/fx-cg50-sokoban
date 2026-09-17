# SHIFT + AC/ON lifecycle

SHIFT + AC/ON requests a main-thread checkpoint followed by `gint_poweroff(true)`.
Tap and release SHIFT, then press AC/ON; holding SHIFT while pressing AC/ON also
works. SHIFT alone and AC/ON alone do nothing. A tapped SHIFT is consumed by the
next fresh non-modifier key, including keys unused by the game; a second tap
cancels it. ALPHA-modified AC/ON does not power off. Only a fresh AC/ON down event
can request OFF. HOLD, duplicate down, and stale repeat events cannot repeat it.

The shortcut is processed before screen/modal dispatch, including Main, Level
Select, play, INIT, Congratulations, load notice, and save-error screens. It does
not accept INIT or advance a completed level. Transitions and OS resumes clear
pending modifiers and block physically held keys, including SHIFT and AC/ON,
until release. Arrow repeats retain the existing 500 ms / 125 ms profile.

## Source and API evidence

The read-only DIFF EQ reference uses `getkey()` in `src/ui/common.c::take_key()`
and `getkey_opt(GETKEY_DEFAULT & ~GETKEY_MENU, ...)` in its polling paths. It
inherits power-off from gint rather than calling a custom DIFF EQ power routine.
The installed gint 2.11.0 headers/source were inspected:

- `include/gint/keycodes.h`: `KEY_SHIFT = 0x81`, `KEY_ACON = 0x07`.
- `include/gint/keyboard.h`: `GETKEY_DEFAULT = 0x05df` includes
  `GETKEY_POWEROFF = 0x0400` and delayed SHIFT handling.
- `src/keysc/getkey.c::getkey_opt()`: a fresh AC/ON event with SHIFT and without
  ALPHA calls `gint_poweroff(true)`.
- `include/gint/gint.h`: `gint_poweroff(bool show_logo)` resumes execution after
  ON. The documented `true` choice avoids immediate reboot from held AC/ON.
- `src/kernel/world.c::gint_poweroff()`: copies VRAM and calls the supported OS
  `__PowerOff` routine inside `gint_world_switch()`.

SOKOBAN continues to read raw `keydev_read()` events with only
`KEYDEV_TR_REPEATS`; its small host-testable input layer tracks tap/held modifiers.
Using automatic `getkey()` power handling would enter OFF before the application
can checkpoint. The native action uses the same supported `gint_poweroff(true)`
mechanism as the inspected DIFF EQ / gint path.

## Persistence and failures

If progress is dirty, the current play state is checkpointed into the existing
per-level progress structure. The existing synchronous two-slot save then runs
to completion, including close and readback verification, before the power hook
is invoked. Every native BFile operation, including failed-close retries, stays
inside the storage transaction's gint OS world switch. No I/O runs in a keyboard
interrupt. The save format, slots, identity, game engine and UNDO representation
are unchanged. A clean state performs no storage write, even in play.

Each OFF request attempts at most one save. On failure, the valid previous slot,
dirty RAM progress and generation remain intact; `SokApp.power_save_failed`
records the failure. OFF still proceeds without a save dialog blocking it. This
RAM flag describes the latest power checkpoint attempt and resets on a later
successful/clean OFF request; it is not serialized or a claim of successful
storage. Existing save-error retry/leave behavior remains available after resume.
A failed OS close is a failed transaction; its bounded cleanup policy is the
existing storage adapter policy, never a mid-write interrupt or forced power cut.

After ON, gint normally returns to the suspended application; the screen and
unsaved RAM are retained and input is gated until release. A genuinely new
SOKOBAN launch loads verified storage and starts at Main. Completion restores its
clear flag; completed boards follow the existing replay-from-start semantics.

## Automated verification and remaining hardware checks

`tests/test_power.c` compiles the actual `src/main.c` against a native API mock
with keycodes verified against the installed headers. It runs real input,
application workflow, codec and two-slot transaction code. It checks tap/held
chords, cancelled/consumed modifiers, ALPHA exclusion, AC-before-SHIFT ordering,
all screens/modals, release barriers, no duplicate save/OFF, clean-state write
avoidance, save/readback/close before OFF, moved/pushed/undone snapshots, clear
flags, finite write failure and preservation of the previous valid slot.
The separate native storage tests exercise the actual BFile adapter/world-switch
boundary and its failure cleanup.

**HARDWARE TEST REQUIRED:** actual calculator power-off and ON, held-key behavior
on the physical scanner, Main/Level/play/INIT/completion shortcuts, subsequent
MENU/EXIT, genuine cold relaunch, restored moves/pushes/UNDO and clear flags, and
practically testable storage failures. Host callbacks do not establish that the
calculator powered off or that its firmware resumed successfully.

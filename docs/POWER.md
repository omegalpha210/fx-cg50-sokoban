# SYSTEM idle settings and SHIFT + AC/ON lifecycle

## Idle policy added in beta.3

`src/system_power.c` reads Auto Power Off, Backlight Duration and normal display
brightness inside one `gint_world_switch()` callback. It runs at launch and
after each MENU/ON return. Duration settings are read-only: the app never calls
their setters or writes SYSTEM setup memory. `src/idle.c` is the host-testable
policy: 10/60 minutes to OFF; 1/2/6 half-minutes to dim. Unexpected values use
10 minutes/30 seconds. Unknown brightness (outside 1..5) suppresses both dim
and restore calls. The CG50 dim request is level **0**, below user level 1.

The main loop reads raw key events non-blockingly, evaluates inactivity with
`rtc_ticks()`, then calls gint `sleep()` when the queue is empty. Gint's existing
128 Hz keyboard scanner wakes the CPU; no new timer is reserved. This follows
the installed scanner's own wait-loop pattern. No storage, rendering, syscall
or application transition occurs in an interrupt. The clock wraps at midnight;
policy arithmetic handles the wrap. MENU/ON resets the clock reference, including
when SYSTEM changed the wall clock. A key waiting in the queue takes priority
over a due OFF. DOWN/UP/HOLD and any physically held key, even an unused key,
reset inactivity. A wake key also performs its normal action.

Dimming/restoration call the transient backlight syscall in the OS world. The
normal brightness is restored before MENU/OFF and on activity. Automatic OFF
calls the same `sok_app_power_off()` main-thread checkpoint as SHIFT+AC/ON,
including every modal. ON rereads settings, resets both deadlines, redraws and
blocks stale held input. This is implemented software behavior; actual LCD
brightness and firmware suspend/resume remain unverified on hardware.

### Primary API evidence and limits

- [CASIO CG50 software manual, System / Power Properties, page 12-2](https://www.casio.com/content/dam/casio/global/support/manuals/calculators/pdf/004-en/f/fx-CG50_Soft_v360_EN.pdf)
  lists the two OFF and three backlight durations and their defaults.
- [libfxcg system declarations](https://github.com/Jonimoose/libfxcg/blob/daa76d9e37758ab5b9b0f614b7e0aa6acfbe2e18/include/fxcg/system.h),
  [GetAutoPowerOffTime syscall](https://github.com/Jonimoose/libfxcg/blob/daa76d9e37758ab5b9b0f614b7e0aa6acfbe2e18/libfxcg/syscalls/GetAutoPowerOffTime.S)
  and [GetBacklightDuration syscall](https://github.com/Jonimoose/libfxcg/blob/daa76d9e37758ab5b9b0f614b7e0aa6acfbe2e18/libfxcg/syscalls/GetBacklightDuration.S)
  establish minutes / half-minutes and indices `0x1e91` / `0x12d9`.
- [TeamFX's original backlight investigation](https://www.cemetech.net/forum/viewtopic.php?t=11908)
  identifies stored light-level getter `0x1e8f` and transient setter `0x0199`.
  The setter does not update the saved normal level, which is the desired behavior.
- [TeamFX's CG50-specific investigation](https://www.casiopeia.net/forum/viewtopic.php?p=14980)
  identifies inactivity level 0 as distinct from CG10/20 behavior. Requesting
  that level through the existing transient setter is the CG50 integration
  choice here; the cited report is **not** a hardware test of this add-in or
  a formal guarantee for every firmware revision. Compare OS/add-in dimming
  on the target calculator before marking this accepted.
- Installed gint 2.11 `src/kernel/syscalls.S` establishes the CG dispatcher
  at `0x80020070`, index in r0, arguments in r4..r7. The four original tiny
  assembly wrappers were inspected in the SH disassembly. The installed
  `src/keysc/getkey.c` does not implement CG idle management; its
  `GETKEY_BACKLIGHT` branch is for monochrome models.

No external implementation is copied, no firmware image is bundled, and no
private OS RAM address or direct LCD/port register write is introduced.

## Manual shortcut

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
storage. After resume, a failed checkpoint displays SAVE FAILED, preserving
the previous screen/modal and dirty RAM. If already in a save-error dialog,
its original pending action is retained. MENU in a save-error dialog can now
retry and request the OS menu; F6 can then leave without saving. A pending
completion retains its win dialog after the OS menu returns.
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
boundary and its failure cleanup, including one-shot errors at all 48 save and
57 load calls in the selected transaction fixtures. Persistent close failure
remains bounded and retains a pending handle for cleanup at the next boundary;
this does not establish that a firmware syscall cannot hang internally.

The idle suite sweeps every 128 Hz tick through all six setting combinations at
three start times, including midnight (18 runs). Native-entry tests cover
automatic OFF in all screens/modals, dim/wake, held/unused keys, clean saves,
previous-slot preservation on failure, startup load notices, unknown brightness,
changed SYSTEM settings/clock after MENU, and no immediate second OFF after ON.
The actual native power adapter is separately checked for OS-world boundaries.

**HARDWARE TEST REQUIRED:** actual calculator power-off and ON, held-key behavior
on the physical scanner, Main/Level/play/INIT/completion shortcuts, subsequent
MENU/EXIT, genuine cold relaunch, restored moves/pushes/UNDO and clear flags, and
practically testable storage failures. Host callbacks do not establish that the
calculator powered off or that its firmware resumed successfully.

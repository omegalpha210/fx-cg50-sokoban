# User guide

## Goal and counters

Move the diamond marked YOU in the HUD with the arrow keys. BASIC uses blue,
INTERMEDIATE pink, ADVANCED violet and MASTER mint. Push the orange crates
onto every dark goal dot. Walls and exterior space are impassable. You may
push one crate into an empty floor/goal square; you cannot pull crates or push
two together. Orange crates retain their color on a goal and gain a white mark.
The underlying goal remains when a player or crate leaves it.

CRATES is the number currently on goals / total crates. MOVES counts successful
player steps, including pushes. PUSHES counts crate pushes only. Blocked inputs
change neither counter. Counters are exact 32-bit unsigned values; if MOVES
reaches 4,294,967,295, further moves are refused until undo/restart rather than
wrapping or displaying a false count. Long values use a second HUD line.

## Menus

Main has four text tiles: BASIC 1–15, INTERMEDIATE 16–30, ADVANCED 31–45, MASTER
46–60. These are user-defined groups, not an upstream difficulty claim. All
levels are unlocked. Arrows select in two dimensions, 1–4 open a group
immediately, and EXE / F6 OPEN opens the selected tile. EXIT stays on Main.

Each group shows 15 global level numbers in a 5×3 grid, increasing by rows.
A white cell with colored number is uncleared; a filled cell with white number
is cleared. A separate dark outline is the selector. LEFT/RIGHT traverse the previous/next number across row boundaries and wrap the
whole page. UP/DOWN preserve the column and wrap vertically.
EXE / F6 opens a level; EXIT returns to the original Main group.

The play screen has LEVEL and statistics at left and the entire board at right.
It has no title bar; the full 24 pixels expand the board viewport to268×196.
Main and level-grid headers remain.
Square tiles scale to each original map; there is no scrolling or stretched
board. Green, blue, gold and red walls identify the four groups.

## Undo, restart and adjacent levels

F2 UNDO immediately reverses one successful walking step or push, including
player/crate positions and exact counters. Five successive undos are available
at most. Blocked inputs consume no history. A new move after undo discards that
future; there is no redo. The remaining history is saved with each level.
Control keys act on fresh presses, so holding F2 does not consume all five.

F1 INIT opens `RESTART LEVEL?` with `EXE: RESTART` and `EXIT: CANCEL`. Nothing changes until
EXE. Confirmation resets this level's layout, counters and undo, then checkpoints
it. Its earned completion and other levels remain intact. The panel's top stripe
matches the current group's player color.

F5 LEVEL- / F6 LEVEL+ checkpoints and opens the adjacent global level, including
15→16, 30→31 and 45→46. Level 1 has no predecessor and 60 has no successor;
these controls do not wrap. A saved attempt on the destination resumes.
Unavailable LEVEL-/+ and empty UNDO buttons are muted. Dialogs clear unrelated
softkeys; the save-error dialog labels F6 as SKIP.

## Completion and resume

The last successful push onto the remaining goal marks completion, checkpoints,
and shows `Congratulations!`. EXE opens the next global level, or the level menu
at level 60. EXIT dismisses the dialog to show the completed board, at every
level. Arrows, F1 INIT and F2 UNDO are locked there; INIT/UNDO are gray and the HUD
reads COMPLETED / EXIT: LEVELS. F5/F6 still open adjacent levels within bounds.
Press EXIT again to return to the current level's grid. To replay, reopen that
level from the grid. MENU and SHIFT+AC/ON remain available in the board view.
A release barrier prevents the same held EXIT from immediately opening the grid.

Completion flags never disappear on retry, INIT or UNDO. A completed board is
not stored as an in-progress resume, preventing repeated congratulations at
startup. Selecting the completed level begins a fresh attempt; a later
interrupted attempt is independently resumable.

## Automatic persistence and leaving

There are no SAVE/RECALL menus. Walking and undo update RAM and mark it dirty.
The app checkpoints after INIT, on completion, before play EXIT, before global
level switches, and before MENU from a game/menu. SHIFT + AC/ON also checkpoints dirty progress
once before entering the supported power-off routine. All 60 states are preserved
independently; switching levels does not discard others' progress.

MENU opens the real CASIO MAIN MENU after the checkpoint. Returning to a
suspended execution retains RAM and the current screen. A genuine new execution
loads disk progress and starts at Main. EXIT in the app never returns from
`main()` or terminates the add-in.

A failed save displays `SAVE FAILED`:

- EXE: retry saving the current RAM state.
- F6: perform the pending action without claiming a successful save.
- EXIT: stay in the app. RAM and the dirty state remain for later retry.
- MENU: request CASIO Main Menu with another save attempt; if it fails, EXE
  retries or F6 leaves without saving. A completed board retains its win dialog.

When INIT was already confirmed, EXIT from a save error keeps the restarted
board; it does not roll back INIT. A completion remains earned in RAM even if
saving fails. Leaving without saving may lose that new progress on a genuine
restart. A corrupt save slot is recovered from the other valid slot. If neither
existing slot can be used, a startup notice explains that fresh progress is used.

Files `SOKO_A.dat` and `SOKO_B.dat` are in storage memory root. Back up both while
the app is inactive if desired. Do not substitute DIFF EQ files. Do not assume
forced power loss preserves steps since the last checkpoint. Actual filesystem
and MENU behavior must be checked using [HARDWARE_RETEST.md](HARDWARE_RETEST.md).

## SHIFT + AC/ON

Tap SHIFT then AC/ON, or hold SHIFT while pressing AC/ON, to power off from any
screen or modal. SHIFT alone and AC/ON alone have no game action. A fresh AC/ON
press is required; repeat/HOLD cannot issue another OFF. ALPHA-modified AC/ON is
excluded, following gint's supported shortcut semantics.

Dirty progress is checkpointed through the existing two-slot transaction, with
write/close/readback completed before `gint_poweroff(true)`. Clean state causes
no storage write. If the one attempt fails, dirty RAM and an error flag are
retained and power-off still proceeds; there is no blocking save dialog. That
failure must not be mistaken for persistence. After ON, SAVE FAILED offers
retry/skip/stay and preserves the prior screen or modal behind it.

ON can resume the same suspended execution in gint, keeping the current screen
and RAM with a release barrier. A genuinely new application launch starts at
Main and loads saved per-level progress. Hardware power-off, wake-up and
fresh-launch persistence still require the checklist. See [POWER.md](POWER.md).

## SYSTEM idle settings

Auto Power Off follows SYSTEM's 10 or 60 minutes. Backlight Duration follows
30 seconds, 1 minute or 3 minutes. These settings and normal brightness are read
at launch and after MENU/ON. Dimming requests CG50 idle brightness level 0;
physical equivalence with the firmware is a required hardware test.
Any physical key activity restores brightness and resets both deadlines. Held
and unused keys count; a wake key also performs its normal game action.
All screens and dialogs share these deadlines. Automatic OFF follows the same
save/failure rules as manual OFF. No settings are written back to SYSTEM.
Invalid duration reads fall back to 10 minutes/30 seconds. An unknown normal
brightness disables brightness changes so no unverified restore level is used.

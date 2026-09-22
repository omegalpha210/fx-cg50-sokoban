# Hardware retest — v0.1.0-beta.3

Earlier basic play was reported working. All new beta.3 checks below are
**NOT RUN on hardware**. Record calculator/OS version, package SHA-256,
tester/date and PASS / FAIL / NOT RUN. Back up both SOKOBAN save files before
intentionally testing corruption or storage failure.

## Priority: SYSTEM settings, dimming and auto OFF

1. In SYSTEM select 10-minute OFF / 30-second backlight, normal brightness 3.
2. Launch SOKOBAN. Confirm it dims after 30 seconds and matches the OS idle
   brightness (CG50 level 0, darker than normal level 1).
3. Press an unused key: brightness returns, screen/progress does not change.
   Press an arrow after dimming: brightness returns and the normal action occurs.
4. Repeat with normal brightness 1 and 5. Verify the stored SYSTEM level is unchanged.
5. Repeat Backlight Duration 1 minute and 3 minutes; time the actual thresholds.
6. Hold an unused key past the dim deadline; it must stay awake. Release and
   verify a new full interval. Repeat held arrows against a wall.
7. Leave Main idle: 10-minute OFF must occur without a key. Turn ON; no immediate
   second OFF, and no stale key opens another screen.
8. Repeat with 60-minute OFF. Repeat both OFF choices with every backlight duration.
9. After moving/pushing/undoing, idle to OFF. ON must retain state; fresh launch
   must load the verified checkpoint, including counters/history.
10. Repeat idle OFF from Level Select, INIT, Congratulations, load notice and
    SAVE FAILED. No dialog may trap OFF. Do not accept INIT or advance a level.
11. Reproduce a safely testable save failure before OFF; ON must display
    SAVE FAILED with retry/skip/stay, preserving RAM and the previous valid slot.
12. From that dialog, press MENU: retry or F6 leaves without saving. Return from
    the OS menu and confirm the original modal, especially completion, is retained.
13. Change the two durations and normal brightness in SYSTEM via MENU. Return
    to SOKOBAN; the new settings and a fresh interval must apply. Repeat after
    changing the clock and across midnight.
14. Leave the calculator OFF long enough for genuine fresh launch/RAM loss.
    Verify successful checkpoints restore and failed checkpoints are not claimed saved.

## Priority: player and control clarity

1. Compare level59 and the other 9px boards with larger boards: identify the
   blue diamond separately from dark goal dots and orange crates at native LCD size.
2. Check all four wall palettes, normal/lowest brightness, and a player on a goal.
3. Confirm the YOU legend matches the board marker at every scale.
4. Empty UNDO, level1 LEVEL- and level60 LEVEL+ look disabled and do nothing.
5. INIT reads RESTART LEVEL?; EXE restarts, EXIT cancels. Unrelated softkeys
   disappear from all dialogs. F6 SKIP on SAVE FAILED matches its displayed choice.

## Retained beta.2 icon checks (all NOT RUN on hardware)

1. Confirm the new 10px-grid SOKOBAN icon appears in CASIO Main Menu.
2. Check the crate has the same visible outer size as one wall cell.
3. Confirm the orange crate does not look oversized, including beside adjacent walls.
4. Identify the small black player without relying on the enlarged host preview.
5. Identify the hollow goal marker separately from the crate.
6. Confirm player → crate → target reads as a Sokoban push puzzle.
7. Verify a clear physical gap between artwork and the OS `SOKOBAN` label.
8. Check there is no top clipping or lost wall/player pixel.
9. Select and deselect the app; ensure the artwork and selection remain visible.
10. Compare visual size with adjacent Main Menu icons; it should not look too small.
11. Compare with DIFF EQ: artwork and OS label should read as separate elements.

Current host bounds are `(6,2)..(85,41)` in 92×64, leaving 22 lower rows. These
measurements do not establish the OS label position. The label mock is illustrative,
not a firmware rendering. Only after hardware feedback, consider a further 1–2px
shift or a small grid-preserving layout adjustment if the gap is still insufficient.
Do not mark these checks passed from PNG/package tests.

## Navigation

1. Main RIGHT follows1→2→3→4→1, including2→3 across the row.
2. Main LEFT follows1→4→3→2→1.
3. Main UP/DOWN stays in its column and wraps;1↔3 and2↔4.
4. BASIC RIGHT5→6 across the first row.
5. BASIC RIGHT10→11 across the second row.
6. BASIC RIGHT15→1 wraps the whole page.
7. LEFT6→5,11→10 and1→15 reverse those boundaries.
8. Repeat all boundaries in the other three groups. Confirm global numbers,
   UP/DOWN same-column wrap, numeric Main shortcuts and EXE/F6 OPEN.

## Gameplay layout

9. Gameplay has no SOKOBAN/group title; Main and level-grid titles remain.
10. LEVEL, CRATES, MOVES and PUSHES retain readable hierarchy at the raised HUD.
11. Inspect largest level59 (29×20,36 crates); its square tiles are9px now.
12. Inspect other minimum-tile boards10,19,20,22,30,54,58 and goal-rich level12.
13. Boards are horizontally/vertically centered in the new right viewport;
    no HUD overlap, board clipping or unused former title strip.
14. INIT/UNDO colors, F3/F4 blanks, LEVEL-/+ labels and softkey strip are intact.
15. INIT and Congratulations panels appear centered in the gameplay usable area
    and do not trigger a second screen from the same held EXE/EXIT press.

## Main Menu icon

16. Both selected/unselected SOKOBAN icons use the new equal-size wall/crate grid.
17. Actual OS `SOKOBAN` label is visually separate from the artwork's lower edge.
18. No top clipping or loss of important pixels; contrast is acceptable in both
    selection states. Host92×64 PNG inspection cannot establish this OS spacing.

## SHIFT + AC/ON

19. From Main, test tapped SHIFT then AC/ON and held SHIFT+AC/ON.
20. Repeat from each level-select group.
21. From gameplay after walking, request OFF; no save dialog should trap it.
22. Repeat after a crate push; verify counters before shutdown.
23. Repeat after UNDO; note remaining history and positions.
24. Repeat from INIT confirmation without accepting the restart.
25. Repeat from a completed-level modal without advancing it.
26. Verify the calculator actually powers off exactly once. SHIFT alone,
    AC/ON alone, AC held/HOLD, and ALPHA+SHIFT+AC must not issue unintended OFF.
27. Press ON. If gint resumes the same execution, confirm RAM/screen continuity
    and release gating; no immediate movement, repeated OFF or modal acceptance.
28. Start a genuinely new SOKOBAN execution. It should open Main, not force the
    old screen; select each previously interrupted level.
29. Player/crates, MOVES/PUSHES and per-level progress match the successful OFF
    checkpoint. An intentionally failed save is not promised to restore new data.
30. Saved undo history and persistent clear flags also restore. Cleared levels
    begin a fresh attempt, with no repeated Congratulations resume loop.

## Other lifecycle and failure checks

31. Play EXIT checkpoints to its level grid, grid EXIT returns to the original
    Main group, and Main EXIT remains in the app.
32. MENU checkpoints and opens actual CASIO MAIN MENU once; return retains RAM
    and blocks held inputs. Repeat from Main, grid, INIT and completion.
33. LEVEL-/+ still crosses15↔16,30↔31,45↔46 and stays bounded at1/60. Independent
    interrupted states and undo histories must not mix.
34. If practical with backed-up disposable saves, test a corrupt newest slot,
    two unusable slots, insufficient storage and OFF after save failure. Normal
    saves offer retry/stay/without-save; OFF tries once, retains RAM/error state,
    and proceeds. Previous valid file must remain usable; no mid-write forced cut.

## LCD/input/resources

35. Distinguish9px wall/floor/goal/orange crate/white arrival mark/black player on
    the physical LCD, including gold walls and white completed-grid numbers.
36. Direction repeat begins after approximately500ms then remains at125ms without
    acceleration. Blocked moves do not alter counters/history.
37. Hold F2/F1/F5/F6/EXE/EXIT and cross modals/MENU/OFF. No duplicate control action,
    next-screen movement, or swallowed fresh press after a late release/HOLD.
38. Recheck icon contrast and OS label gap. Exercise repeated MENU/OFF/save cycles
    and measure actual stack/heap headroom if instrumentation is available. The
    compiler's212-byte maximum project frame is not whole-program stack peak.

Hardware confirmation of actual power-off, wake-up, cold-launch saves, full
Fugue behavior, LCD spacing and stack/heap margins remains NOT RUN until these
results are recorded. Do not mark mocked native callbacks as real device tests.

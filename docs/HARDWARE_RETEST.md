# Hardware retest — v0.1.0-beta.1

The owner reports that basic play worked on an fx-CG50 and that the prior icon
appeared too low beside the OS label. These are user observations, not a complete
hardware acceptance run. The new navigation/layout/icon/power changes below are
**HARDWARE RETEST REQUIRED**. Record calculator/OS version, local package SHA-256,
tester/date and PASS / FAIL / NOT RUN for each step. Preserve existing save files
before intentionally testing corruption or storage failure.

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

16. Both selected/unselected SOKOBAN artwork appear3px higher, without shrinking.
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

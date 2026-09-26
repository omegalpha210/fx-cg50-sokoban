# SOKOBAN for CASIO fx-CG50

![SOKOBAN app icon](assets/icon-uns.png)

A native C / fxSDK / gint Sokoban add-in with 60 levels, five-step undo,
independent per-level progress and SYSTEM idle power settings.
**v0.1.0-beta.4** adds four player colors and a completed-board view.
[한국어 설명](README_KO.md)

## Screens

These images use the **actual application renderer and the pinned game maps**.
They are pixel-exact 396×224 host captures, not calculator photographs or CPU
emulation. The level 1 success screen comes from replaying 394 legal moves
from the original starting position. [Capture details](docs/screenshots/README.md)

| Main | BASIC level menu, after clearing level 1 |
|---|---|
| ![Main](docs/screenshots/main.png) | ![BASIC levels](docs/screenshots/levels-basic.png) |

| BASIC · level 1 · blue player | INTERMEDIATE · level 16 · pink player |
|---|---|
| ![BASIC gameplay](docs/screenshots/play-basic.png) | ![INTERMEDIATE gameplay](docs/screenshots/play-intermediate.png) |
| **ADVANCED · level 31 · violet player** | **MASTER · level 59 · mint player** |
| ![ADVANCED gameplay](docs/screenshots/play-advanced.png) | ![MASTER gameplay](docs/screenshots/play-master.png) |

<details>
<summary>Other group menus</summary>

| INTERMEDIATE | ADVANCED |
|---|---|
| ![INTERMEDIATE levels](docs/screenshots/levels-intermediate.png) | ![ADVANCED levels](docs/screenshots/levels-advanced.png) |

![MASTER levels](docs/screenshots/levels-master.png)

</details>

| Restart confirmation | Level completed |
|---|---|
| ![Restart level 1](docs/screenshots/restart-basic.png) | ![Congratulations](docs/screenshots/win-basic.png) |

Press **EXIT on Congratulations to view the completed board**. Movement, INIT
and UNDO are locked; INIT and UNDO appear gray. LEVEL− / LEVEL+ still work.
Press EXIT again to return to the level menu; reopening the level starts a
fresh attempt while keeping its earned clear marker. EXE on Congratulations
opens the next level, or the level menu at level 60.

![Completed board with INIT and UNDO disabled](docs/screenshots/completed-basic.png)

## Build and install locally

The [GitHub prerelease](https://github.com/omegalpha210/fx-cg50-sokoban/releases/tag/v0.1.0-beta.4)
is **source-only**. It includes the selected README screenshots requested by the
project owner. Raw/generated map data and the map-containing `.g3a` are not
published. Map redistribution permission remains unconfirmed; the selected
screenshots do not change the maps' separate rights. Own code is [MIT](LICENSE).
See [asset provenance](docs/ASSET_PROVENANCE.md) and
[third-party notices](THIRD_PARTY_NOTICES.md).

Use an existing fxSDK 2.11 / gint 2.11 / SH GCC installation. Configure PATH or
`SOKOBAN_SDK_ROOT` and Python 3 with Pillow; see [DEVELOPMENT.md](docs/DEVELOPMENT.md).
Explicitly obtain the pinned inputs and build:

```sh
python3 tools/fetch_maps.py
python3 tools/import_maps.py
bash tools/build.sh
bash tools/test.sh
```

Fetch verifies immutable revisions, sizes and SHA-256 hashes. Ordinary builds
never download or update maps. Obtaining inputs does not establish permission
to redistribute them or a resulting binary.

Copy the locally built `dist/SOKOBAN.g3a` to the fx-CG50 in USB storage mode,
disconnect safely, and select SOKOBAN from CASIO MAIN MENU.
`dist/SHA256SUMS.txt` records its checksum. App identity is `@SOKOBAN`;
save files are `SOKO_A.dat` and `SOKO_B.dat`. DIFF EQ files are independent.

## Controls and progress

| Key | Action |
|---|---|
| Menu arrows | LEFT/RIGHT traverse numbers across row ends and wrap the page. UP/DOWN wrap within the same column. |
| 1–4 on Main | Open a group immediately. |
| EXE / F6 OPEN | Open the selected group or level. |
| Play arrows | Move or push one crate. Locked on a completed board. |
| F1 INIT | Restart confirmation: EXE restarts, EXIT cancels. Disabled on a completed board. |
| F2 UNDO | Undo up to five successful moves or pushes. Disabled on a completed board. |
| F5 LEVEL− / F6 LEVEL+ | Save and open the adjacent global level; no 1↔60 wrap. |
| EXIT | Dismiss Congratulations to view the board; from play, save and return to the level menu; from the level menu, return to Main. |
| MENU | Save and return to the actual CASIO MAIN MENU. |
| SHIFT + AC/ON | Attempt one dirty checkpoint, then power off. A failed save does not trap power-off. |

BASIC 1–15, INTERMEDIATE 16–30, ADVANCED 31–45 and MASTER 46–60 are project-defined
groups, not an upstream difficulty ranking. All levels are available immediately.
The diamond retains the same shape at every tile size; its color and the restart
dialog's accent follow the group. The HUD identifies it as YOU during play.

Each level keeps its own state and five undo records. Completion, INIT, play
EXIT, level changes, MENU and dirty power-off checkpoint progress. Two save
slots preserve the last valid save through failed writes. Failed normal saves
offer retry/stay/continue without saving; failed OFF saves offer recovery after
ON. Forced power loss cannot preserve every uncheckpointed step.
[Full user guide](docs/USER_GUIDE.md)

SYSTEM Auto Power Off (10/60 minutes) and Backlight Duration (30 seconds/1/3
minutes) are read on launch and after MENU/ON. Any key, including an unused or
held key, resets inactivity. Activity restores normal brightness; OS preferences
are not modified. [Power details](docs/POWER.md)

## Validation

Host/UBSan tests cover game rules, storage failures, input transitions, idle
power and completed-board controls. The actual level 1 replay checks the
completion dialog and disabled buttons. The native package is compiled with
strict warnings and validated before publication.

![Four player palettes at every runtime size, enlarged 3x](docs/public-captures/player-sizes-3x.png)

**Hardware retesting is still required** for LCD colors, physical key timing,
SYSTEM brightness and real OFF/ON persistence. Host captures do not establish
physical-device behavior. [Exact results](docs/ACCEPTANCE.md) ·
[Hardware checklist](docs/HARDWARE_RETEST.md) ·
[오류·안정성 상세 검증](docs/STABILITY_KO.md)

# Acceptance — v0.1.0-beta.4

Beta.4 uses distinct blue/pink/violet/mint players for the four groups, with
matching restart-panel accents. EXIT on Congratulations shows the completed
board; movement/INIT/UNDO are locked and INIT/UNDO are gray. Adjacent levels,
MENU, EXIT and manual/automatic OFF remain available. Reopening the level from
the grid starts a playable new attempt with the earned clear marker preserved.

README images now show actual unchanged pack levels 1/16/31/59 through the same
C renderer used by the add-in. Level 1 restart/completion are captured from legal
input, not substituted terrain, box positions or counters. SAVE FAILED is removed
from the showcase, while its runtime handling and tests remain. These are host
captures, not calculator photographs or hardware validation.

The public baseline is `c04559eebf0ba7c60154b70d21935d78dbfa1e10` (beta.3).
Publication preserves its ancestry and all existing tags. The owner's actual-map
README request authorizes only the selected screenshots; upstream map rights
remain separate. Development history, raw/generated maps, other map captures and
bundled binary remain local. The prerelease is source-only.

## Automated evidence

| Check | Result |
|---|---|
| Python | 41 tests passed: importer/package/icon/layout/publication, including stale-version rejection and the exact screenshot allowlist. |
| C/UBSan | 10/10 suites: engine, storage, workflow, idle, renderer, real-map showcase, native storage, native entry/power, native power adapter, fixture renderer. |
| Engine | 359,247 assertions; 76,275 successful randomized moves and 1,500 pushes/UNDO across all 60 maps. |
| Workflow | 333,615 assertions, including 100,000 mixed events with save failures, screen changes and OFF. Completed-board checks cover levels 1/15/16/30/31/45/46/59/60, held EXIT, locked actions, boundary navigation, clear retention and failed-save recovery. |
| Real-map replay | Level 1 solved from its original start by 394 legal moves / 134 pushes; success, EXIT view, disabled-button pixels and fresh replay verified. 12 distinct README images from 13 captures; BASIC menu is recaptured with the genuinely earned clear flag. |
| Native storage | Errors injected at 48 save + 57 load OS-call boundaries; backup/RAM/generation/close/retry verified. 9,026 BFile calls in 215 OS transactions. |
| Idle | All ticks through six duration combinations at three start times, including midnight; 18 complete sweeps. |
| Native entry | Manual/automatic OFF includes the new completed-board view; held/unused keys, dim/restore, settings after MENU, startup notices, clean writes and finite failure paths pass. |
| OS power adapter | Setting reads and transient brightness calls occur inside the OS world; invalid brightness rejected. Power implementation unchanged from beta.3. |
| SH build | Strict compile/link; zero compiler warnings. |
| Package | 16 G3A checks; version 00.01.0004; both icon records match retained beta.2 PNGs. |
| Renderer | 87 full-pack captures; all boards fit and no out-of-bounds drawing. Four player palettes at every 9..19px size retain their fill/border. All four restart accents match their player color. |
| ASan | Not verified; existing host runtime stalls before main. UBSan passes. |

The exact public candidate is rebuilt from fresh build directories with ignored
pinned map inputs. Native payload/package must match development output
byte-for-byte. Source ZIP contents are checked against the tag. No map-bundled
binary is uploaded. Publication evidence is retained outside the public snapshot.

## Native size and package

| Measurement, bytes | beta.3 | beta.4 |
|---|---:|---:|
| ELF text | 48,156 | 48,440 |
| data | 512 | 512 |
| BSS | 12,432 | 12,432 |
| Largest own single stack frame | 212 | 212 |
| SokApp | 5,812 | 5,812 |
| SokProgress | 5,648 | 5,648 |
| Save workspace | 5,520 | 5,520 |
| Maximum encoded save per slot | 3,336 | 3,336 |
| Local G3A | 77,344 | 77,628 |

No new timer, framebuffer, allocation or persistent state flag is introduced.
The completed-board lock is derived from the current board, so an earned clear
flag does not lock a subsequent replay. Rules, map topology/order, save format
version 1, SYSTEM power integration and launcher icons are unchanged.
Single-frame size is not total stack peak or available RAM. See [MEMORY.md](MEMORY.md).

Local `dist/SOKOBAN.g3a` SHA-256:
`347a5378be6ebaf48b4852828dd3abeb86227512b334cfa84d82f399dfb9c036`.
Native payload SHA-256:
`a60171a7c953354e1bd10f7ae9c28c4cffcad0af582eb93e858590a95ccd3884`.
`SOURCE_DATE_EPOCH=1789660800` fixes package header time for candidate comparison.
These identify local validation output, not downloadable release binaries.

## Physical limits

No host crash or UBSan failure was reproduced. Actual LCD palette contrast,
physical key timing, firmware brightness, OFF/ON and cold-launch persistence
remain **HARDWARE TEST REQUIRED**. Host mocks and renderer images do not establish
that firmware calls cannot hang or that the calculator is crash-free.

The retained CG50 level-0 dimming assumption and restore call require physical
verification; sources and inference limits are in [POWER.md](POWER.md). The
inherited gint emergency abort chord and error paths are in
[STABILITY_KO.md](STABILITY_KO.md). Use [HARDWARE_RETEST.md](HARDWARE_RETEST.md)
for the full physical checklist. No beta.4 hardware pass is claimed.

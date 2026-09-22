# Acceptance — v0.1.0-beta.3

Beta.3 adds SYSTEM idle settings, a blue diamond player and clearer modal/control
hints. Manual and automatic OFF share a checkpoint; failed OFF saves now show
SAVE FAILED after ON. MENU also works from save errors, retaining completion
state. Rules, 60 pinned maps, save version 1, app/save identity and beta.2 launcher
icons are preserved. Hardware brightness and suspend/resume remain unverified.

The public baseline is `ccaaa5ea5ec8af9571fb76ca3a62ec5c40523881` (beta.2).
Publication preserves this ancestry and existing tags. Development history,
upstream maps/captures and bundled binary remain local. The release is source
only because map redistribution permission remains unresolved.

## Automated evidence

| Check | Result |
|---|---|
| Python | 40 tests passed: importer/package/icon/layout/publication, including rejection of a correctly checksummed package with stale release version. |
| C/UBSan | 9/9 suites: engine, storage, workflow, idle, renderer, native storage, native entry/power, native power adapter, public renderer. |
| Engine | 359,247 assertions, 76,275 successful randomized moves and 1,500 pushes/UNDO across all 60 maps. |
| Workflow | 333,106 assertions, including 100,000 mixed events with save failures, screen changes and OFF. |
| Native storage | Errors injected at 48 save + 57 load OS-call boundaries; backup, RAM, generation, close and retry verified. 9,026 BFile calls in 215 OS transactions. |
| Idle | All ticks through six duration combinations at three start times, including midnight; 18 complete sweeps. |
| Native entry | Manual/automatic OFF in all screens/modals, held/unused keys, dim/restore, changed settings/clock after MENU, startup notices, unknown brightness, clean writes avoided, finite save failure. |
| OS power adapter | Three setting reads and transient brightness calls occur inside the OS world; out-of-range brightness rejected. Four SH syscall wrappers inspected in disassembly. |
| SH build | Strict compile/link, zero compiler warnings. |
| Package | 16 G3A checks; version 00.01.0003; both icon records match retained PNGs. |
| Renderer | 87 full-pack captures, no out-of-bounds drawing; six public own-fixture UI captures and every player size 9..19px checked. |
| ASan | Not verified; existing host runtime stalls before main. UBSan passes. |

The public candidate is rebuilt from fresh build directories after hydrating
ignored pinned map inputs. The native payload/package must match the development
build byte-for-byte before publishing. Source ZIP contents are checked against
the tag; no map-bundled binary is uploaded. Local publication evidence is stored
outside the public snapshot.

## Native size and package

| Measurement, bytes | beta.2 | beta.3 |
|---|---:|---:|
| ELF text | 46,512 | 48,156 |
| data | 464 | 512 |
| BSS | 12,400 | 12,432 |
| Largest own single stack frame | 212 | 212 |
| SokApp | 5,812 | 5,812 |
| SokProgress | 5,648 | 5,648 |
| Save workspace | 5,520 | 5,520 |
| Maximum encoded save per slot | 3,336 | 3,336 |
| Local G3A | 75,652 | 77,344 |

No extra timer, framebuffer or per-move allocation is introduced. Idle policy
and cached settings add 32 bytes of application BSS. Single-frame size does not
measure the total stack peak or available RAM. See [MEMORY.md](MEMORY.md).

Local `dist/SOKOBAN.g3a` SHA-256:
`ce79a397f1992e9e541a36a2bd1dbc8ab2055f7a5e1d9fdfe07c65213ba4386b`.
Native payload SHA-256:
`dabc1583b52690d9431693bbdc02c77b23520e1e7d816fa34031ffdc37f54bd0`.
`SOURCE_DATE_EPOCH=1789660800` fixes package header time for candidate comparison.
These identify local validation output, not downloadable release binaries.

## Findings and physical limits

Two visible failure paths are improved: failed OFF checkpoints are surfaced on
resume, and a save-error modal no longer ignores MENU. Retargeting a failed
completion to MENU preserves Congratulations after return. No host crash or
UBSan failure was reproduced in the checks above. This does not establish that
firmware calls cannot hang or that the calculator is crash-free.

The CG50 dim request uses level 0 based on CG50-specific investigation; its
behavior through the transient syscall, actual normal-brightness restoration,
all SYSTEM time choices, physical scanner timing, storage failures and real
OFF/ON or cold relaunch are **HARDWARE TEST REQUIRED**. Sources and inference
limits are explicit in [POWER.md](POWER.md). The inherited gint emergency abort
chord is documented in [the detailed Korean stability/error audit](STABILITY_KO.md).

The full physical checklist is [HARDWARE_RETEST.md](HARDWARE_RETEST.md). No physical
beta.3 pass is inferred from host mocks or renderer images.

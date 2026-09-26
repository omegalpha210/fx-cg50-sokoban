# README screenshot provenance

These 396×224 PNGs come from the actual C application and renderer linked to the
unchanged pinned map pack. They are host captures, not photos of a calculator.
They replace the earlier independently authored illustration map in the README.

- BASIC: level 1; INTERMEDIATE: level 16; ADVANCED: level 31; MASTER: level 59.
- Menus are driven by the real app input/state machine.
- Level 1 restart, congratulations and completed-board images come from a replay
  of 394 legal moves / 134 pushes from its original start. The project generated
  this non-optimal solution for capture verification; no boxes are relocated by
  a fixture, and counters are not assigned for display. The BASIC level menu
  shows the completion flag earned by this replay.
- `tests/showcase.c` generates the PPMs. `tools/showcase.py` converts them to PNG
  without changing pixels. `manifest.json` records the resulting hashes.
- No save-error screen is included in the README gallery. Failure paths remain
  tested separately.

The owner explicitly requested these actual-map previews on 2026-09-26. This is
a narrow publication instruction for these images, not evidence of an upstream
license. Map layouts retain separate upstream rights and are **not covered by
the project's MIT license**. See [map provenance](../ASSET_PROVENANCE.md).
Raw/generated map data, map-bundled binaries and all other full-pack captures
remain excluded from the public repository. No wider permission is inferred.

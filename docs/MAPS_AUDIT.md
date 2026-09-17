# Map audit

Generated deterministically by `python3 tools/import_maps.py`. Check without changing files:
`python3 tools/import_maps.py --check`. Both commands use local files only.

Upstream: [begoon/sokoban-maps](https://github.com/begoon/sokoban-maps/tree/7aacdd777e383c172fd1879b31999748d12404f1).
Pinned revision: `7aacdd777e383c172fd1879b31999748d12404f1`.
Plain text SHA-256 / map-pack identifier: `9e4814fc9172a8aa0dcc0d049017f62d640a65c193d62dea1602ce05bcbd6f68`.
The manifest in `assets/maps/` pins exact source URLs and SHA-256 values. Downloaded
text, README and extractor are retained only in local builds and excluded from the
public snapshot. No DOS executable was downloaded or executed.

All **60** maps pass structural validation in original order (IDs 1–60).
Maximum width **29**, height **20**, board cells **580**,
crates/goals **36**. Level 59 reaches all four maxima (29 × 20; 36 crates).
There are 17586 total rectangle cells. Packed terrain occupies
**4412 bytes**, initial crate coordinates **2136 bytes**,
plus 60 read-only map descriptors and the 32-byte pack identifier.

The original decoder explicitly maps `X` to wall, space to empty space, `*` to crate,
`.` to goal, and `&` to crate already on a goal. Its epilogue adds `@` for the player.
These are exactly the six symbols present in the source. `&` contributes once to
both crate count and goal count; its terrain remains GOAL independently of occupancy.

The importer preserves leading spaces, level order, dimensions, and every original
metadata line. Metadata width is authoritative: short rows are padded to that width,
and existing trailing spaces are retained. Rows wider than metadata and mismatched
height are rejected. Boundary-connected non-wall cells are flood-filled; all must
be plain spaces and become VOID. Missing-row padding must belong to that exterior.
Enclosed spaces become FLOOR. No level is cropped or otherwise repaired.

Validation rejects reordered/duplicate/missing IDs, malformed metadata, invalid
dimensions, inconsistent offsets/length, unknown symbols, player count other than
one, unequal or zero crate/goal counts, and occupants/goals exposed to the exterior.
Generated terrain is two bits per cell, four row-major cells per byte, low bits first.
Builds consume locally generated const data; no runtime map text parser or network access
is required. Public source users explicitly fetch the pinned inputs once before import. The pack identifier hashes the unchanged source bytes, including metadata.

BASIC (1–15), INTERMEDIATE (16–30), ADVANCED (31–45), MASTER (46–60) are
user-requested groups. No claim is made that upstream orders these maps by difficulty.
Structural validation is **not** a claim that all 60 maps were solved; no solver was run.
Map redistribution permission remains unconfirmed; see `ASSET_PROVENANCE.md`.

Coordinates in errors are one-based columns/rows of the original metadata rectangle.
The machine-readable full audit is `assets/maps/statistics.json`; it includes source
row lengths, symbols, offsets, original metadata, player position and cell counts.

| Level | Width | Height | Crates | Goals | Initially on goal | VOID | Packed terrain bytes |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | 22 | 11 | 6 | 6 | 0 | 102 | 61 |
| 2 | 14 | 10 | 10 | 10 | 0 | 8 | 35 |
| 3 | 17 | 10 | 11 | 11 | 0 | 56 | 43 |
| 4 | 22 | 13 | 20 | 20 | 0 | 106 | 72 |
| 5 | 17 | 13 | 12 | 12 | 0 | 79 | 56 |
| 6 | 12 | 11 | 10 | 10 | 0 | 16 | 33 |
| 7 | 13 | 12 | 11 | 11 | 0 | 25 | 39 |
| 8 | 16 | 17 | 18 | 18 | 0 | 77 | 68 |
| 9 | 17 | 18 | 15 | 15 | 0 | 129 | 77 |
| 10 | 21 | 20 | 34 | 34 | 0 | 112 | 105 |
| 11 | 19 | 15 | 14 | 14 | 0 | 103 | 72 |
| 12 | 13 | 16 | 34 | 34 | 18 | 47 | 52 |
| 13 | 20 | 13 | 18 | 18 | 0 | 37 | 65 |
| 14 | 17 | 13 | 15 | 15 | 0 | 28 | 56 |
| 15 | 17 | 17 | 15 | 15 | 1 | 94 | 73 |
| 16 | 14 | 15 | 15 | 15 | 1 | 56 | 53 |
| 17 | 18 | 16 | 18 | 18 | 0 | 62 | 72 |
| 18 | 22 | 13 | 13 | 13 | 0 | 60 | 72 |
| 19 | 28 | 20 | 20 | 20 | 0 | 218 | 140 |
| 20 | 20 | 20 | 29 | 29 | 2 | 83 | 100 |
| 21 | 16 | 14 | 6 | 6 | 0 | 49 | 56 |
| 22 | 22 | 20 | 35 | 35 | 0 | 71 | 110 |
| 23 | 25 | 14 | 18 | 18 | 0 | 117 | 88 |
| 24 | 21 | 19 | 23 | 23 | 0 | 92 | 100 |
| 25 | 23 | 17 | 21 | 21 | 0 | 75 | 98 |
| 26 | 15 | 15 | 13 | 13 | 0 | 46 | 57 |
| 27 | 23 | 13 | 20 | 20 | 0 | 48 | 75 |
| 28 | 15 | 17 | 20 | 20 | 0 | 54 | 64 |
| 29 | 24 | 11 | 16 | 16 | 0 | 44 | 66 |
| 30 | 14 | 20 | 18 | 18 | 0 | 43 | 70 |
| 31 | 15 | 12 | 13 | 13 | 0 | 30 | 45 |
| 32 | 18 | 16 | 20 | 20 | 0 | 89 | 72 |
| 33 | 13 | 15 | 15 | 15 | 0 | 59 | 49 |
| 34 | 12 | 15 | 15 | 15 | 0 | 0 | 45 |
| 35 | 20 | 16 | 17 | 17 | 0 | 44 | 80 |
| 36 | 18 | 19 | 21 | 21 | 0 | 84 | 86 |
| 37 | 21 | 15 | 20 | 20 | 0 | 81 | 79 |
| 38 | 14 | 15 | 14 | 14 | 0 | 37 | 53 |
| 39 | 23 | 18 | 25 | 25 | 0 | 123 | 104 |
| 40 | 11 | 11 | 8 | 8 | 0 | 24 | 31 |
| 41 | 20 | 15 | 16 | 16 | 0 | 107 | 75 |
| 42 | 13 | 18 | 24 | 24 | 0 | 36 | 59 |
| 43 | 17 | 16 | 16 | 16 | 0 | 74 | 68 |
| 44 | 25 | 19 | 16 | 16 | 0 | 135 | 119 |
| 45 | 19 | 11 | 9 | 9 | 0 | 35 | 53 |
| 46 | 22 | 17 | 19 | 19 | 0 | 81 | 94 |
| 47 | 19 | 15 | 9 | 9 | 0 | 86 | 72 |
| 48 | 16 | 15 | 12 | 12 | 1 | 77 | 60 |
| 49 | 19 | 16 | 16 | 16 | 3 | 64 | 76 |
| 50 | 21 | 16 | 24 | 24 | 0 | 47 | 84 |
| 51 | 16 | 14 | 17 | 17 | 0 | 41 | 56 |
| 52 | 21 | 14 | 16 | 16 | 3 | 92 | 74 |
| 53 | 13 | 19 | 16 | 16 | 5 | 32 | 62 |
| 54 | 23 | 20 | 22 | 22 | 0 | 7 | 115 |
| 55 | 22 | 15 | 24 | 24 | 0 | 28 | 83 |
| 56 | 14 | 16 | 14 | 14 | 0 | 42 | 56 |
| 57 | 18 | 11 | 16 | 16 | 0 | 41 | 50 |
| 58 | 27 | 20 | 23 | 23 | 4 | 149 | 135 |
| 59 | 29 | 20 | 36 | 36 | 6 | 133 | 145 |
| 60 | 26 | 16 | 27 | 27 | 3 | 89 | 104 |

# Source publication audit

Audit date: **2026-09-17**. This document defines the material eligible for the
public-source candidate; it does not certify legal rights or claim a remote push
has happened. Actual publication status is reported separately.

## Fresh map-rights evidence

Both the pinned revision and current default-branch HEAD of
[begoon/sokoban-maps](https://github.com/begoon/sokoban-maps) are
`7aacdd777e383c172fd1879b31999748d12404f1` (commit dated 2026-03-18).
The audit queried GitHub's authenticated read-only API and inspected public
upstream pages. No inference was drawn from other projects carrying the same maps.

| Check | Observed result and primary evidence |
|---|---|
| Pinned/current tree, including nested files | [Complete recursive tree](https://api.github.com/repos/begoon/sokoban-maps/git/trees/7aacdd777e383c172fd1879b31999748d12404f1?recursive=1) was not truncated and contained no license file. Root entries: `Makefile`, `README.md`, `disasm`, `maps`, `pusher.exe`, `pushermaps.c`, `screenshots`. |
| README | [Pinned README](https://github.com/begoon/sokoban-maps/blob/7aacdd777e383c172fd1879b31999748d12404f1/README.md) describes extraction from DOS `pusher` and operation of the extractor. No redistribution grant or original map author attribution was found. |
| Headers | [Extractor](https://github.com/begoon/sokoban-maps/blob/7aacdd777e383c172fd1879b31999748d12404f1/pushermaps.c) has Alexander Demin's 1998/2012 copyright attribution and format description, with no license grant. Plain/compressed map headers contain data rather than permission notices; Makefile supplies build commands. |
| GitHub license metadata | [Repository metadata](https://api.github.com/repos/begoon/sokoban-maps) returned `license: null`; [default license endpoint](https://api.github.com/repos/begoon/sokoban-maps/license) and [pinned license endpoint](https://api.github.com/repos/begoon/sokoban-maps/license?ref=7aacdd777e383c172fd1879b31999748d12404f1) returned HTTP 404. Metadata alone is not conclusive, so file contents were checked too. |
| Releases/tags | [Releases](https://api.github.com/repos/begoon/sokoban-maps/releases) and [tags](https://api.github.com/repos/begoon/sokoban-maps/tags) returned empty lists. No grant was found in release metadata. |
| Issues and author statements | All two issues and the one comment were inspected: [issue 1](https://github.com/begoon/sokoban-maps/issues/1), [its comment](https://github.com/begoon/sokoban-maps/issues/1#issuecomment-2597328414), [issue 2](https://github.com/begoon/sokoban-maps/issues/2). They concern puzzle solvability and decoding, not redistribution. No relevant permission statement was found. No issue, message or permission request was sent. |
| Repository history metadata | [Seven commit messages](https://api.github.com/repos/begoon/sokoban-maps/commits?per_page=100) describe import, typo fixes, cleanup, screenshots and a solver reference; no grant was identified. The all-state pull-request query was empty. This was not an investigation of every external statement or of rights in the original DOS game. |

**Result:** redistribution permission remains unconfirmed for the map pack and
its transformations. Public GitHub visibility and MIT terms on an independent
clone do not establish rights to these original layouts. The project's MIT
license covers its own code and cannot supply a missing map grant.

## Candidate scope

Include independently authored engine, UI, storage, input/lifecycle code, build
scripts, importer, tests and documentation; original icons and test layouts;
attributed permitted font data; dependency notices; pinned upstream URLs and
hashes. Deliberate local retrieval can restore omitted inputs for testing and
rebuilding without checking them into the public repository.

Exclude these from every public commit/tag, automatic source archive, release
asset and repository preview:

- `assets/maps/upstream/`, including the README's two example layouts and the
  extractor source whose redistribution grant is also absent.
- `src/maps/generated_maps.c` and other packed or serialized upstream boards.
- All `.g3a` packages containing the pack, package archives, and binary-download
  claims or checksums presented as available release assets.
- Existing map captures/contact sheets, including pictures called `fixture` that
  merely alter a real map's state. Use a separately authored layout for gameplay,
  completion and confirmation examples, labeled as actual host renderer output.
- Private paths/configuration, tokens, PDFs/manual extracts, user files, logs,
  build caches/toolchains, DIFF EQ private history and diagnostic artifacts.

The original local commit contains map data, embedded map source, a bundled
binary, map screenshots and machine-specific documentation. Removing them only
from a later commit is insufficient. Preserve local history and make an
independent clean source snapshot with no original commit ancestry. Audit every
file and all reachable history before any remote publication. Ignore patterns are
a second guard, not a way to remove already tracked content.

The initial clean public snapshot is already published as `v0.1.0-beta.1`.
The beta.2 icon update preserves that public commit as its parent; it does not
create another root or import local development history. Audit the allowlisted
overlay in a checkout of the existing public repository, rebuild it with ignored
local inputs, then append a normal commit and a new prerelease tag. Map policy
and all exclusions above remain unchanged.

Do not substitute a repeated test fixture as the product's real 60-level pack.
It exists only for distributable host-rendered screenshots and tests. Ordinary
runtime map loading and local 60-map data remain unchanged.

## Copied utilities and font checks

The two DIFF EQ utility files at public commit
[`8e6f8f653f246427009beabbb8190f83bd2ab4df`](https://github.com/omegalpha210/fx-cg50-diffeq/tree/8e6f8f653f246427009beabbb8190f83bd2ab4df)
are byte-for-byte identical to the originals used from local development commit
`1a16b1b728c7c9e8a0e2491d96fde4b95cdacdbb`. The public revision has an MIT license,
copyright 2026 DIFFEQ contributors. A verbatim
[license copy](third_party/DIFFEQ-LICENSE.txt) and hash are retained; the SOKOBAN
adaptations preserve attribution in [third-party notices](../THIRD_PARTY_NOTICES.md).

The atlas and gint README were compared with installed source at pinned
`badbd0fd2bd8ac796fd55d49b93691741bd8a139`; both match exactly. The README allows
sharing and modification, and no separate atlas restriction was found. These
terms support retaining the attributed font and rendering original UI/fixture
screenshots. They do not clear upstream map geometry in a picture.

## Validation boundary

Run host/UBSan tests, map integrity checks, strict SH build, package checks, save
round-trip and icon generation from the exact clean source candidate after
explicitly restoring pinned local-only inputs. Generated map data must remain
ignored and the candidate's tracked tree unchanged. These checks prove build
behavior, not redistribution permission.

With map rights unresolved, the eligible prerelease is source-only. A bundled
`.g3a` asset and its SHA-256 upload/download verification are withheld. Do not
report a withheld upload as successful release-asset verification. Hardware
power-off, LCD layout/icon placement and resumed saves still require calculator
retesting independently of source publication.

# Asset and code provenance

Audited 2026-09-17. The project's [MIT license](../LICENSE) covers project-authored
source, documentation and original graphics. It does not grant rights to upstream
maps, the gint font, or other separately identified dependencies. Map
redistribution permission remains **unconfirmed**. The public-source candidate
excludes upstream map files, generated map data and the complete `.g3a` binary.
The beta.4 README request makes a narrow exception for selected real-map
screenshots; this is owner publication scope, not new upstream permission.
This records inspected evidence, not a legal certification.
See [publication audit](PUBLICATION_AUDIT.md) and
[third-party notices](../THIRD_PARTY_NOTICES.md).

## Maps and extraction origin

The 60 local maps come from [begoon/sokoban-maps](https://github.com/begoon/sokoban-maps),
pinned to revision
[`7aacdd777e383c172fd1879b31999748d12404f1`](https://github.com/begoon/sokoban-maps/tree/7aacdd777e383c172fd1879b31999748d12404f1).
The same revision was current HEAD at this audit. Original topology, symbols,
order, dimensions and metadata are preserved.

| Referenced upstream file | SHA-256 | Bytes |
|---|---|---:|
| [Plain maps](https://raw.githubusercontent.com/begoon/sokoban-maps/7aacdd777e383c172fd1879b31999748d12404f1/maps/sokoban-maps-60-plain.txt) | `9e4814fc9172a8aa0dcc0d049017f62d640a65c193d62dea1602ce05bcbd6f68` | 26,892 |
| [README](https://raw.githubusercontent.com/begoon/sokoban-maps/7aacdd777e383c172fd1879b31999748d12404f1/README.md) | `92a329dd88b45439eddad7bb23dd9f381d61da5fb227f8dec6e9beaec575516d` | 2,283 |
| [Extractor reference](https://raw.githubusercontent.com/begoon/sokoban-maps/7aacdd777e383c172fd1879b31999748d12404f1/pushermaps.c) | `1d15ec42c1ed749d0ed0d4e01932b662cc65736efe3f3554c82c5b7ab95b723e` | 5,844 |

The upstream README attributes extraction to the DOS game `pusher`; the extractor
identifies Alexander Demin, copyright 1998 and 2012. Neither establishes the
original map authors or a map redistribution grant. No license file occurs in the
complete inspected repository tree; the README, source header, release metadata
and repository issues provide no explicit permission. The license API reports no
recognized license. This is about the inspected evidence, not a claim that no
permission could exist elsewhere. Public visibility and unrelated clones are not
used as permission evidence.

`tools/import_maps.py` is project-authored parser and validator code. Its generated
`src/maps/generated_maps.c` transforms upstream content and retains the maps'
unresolved rights status. The upstream README itself contains two map layouts and
is excluded from the public-source candidate, together with the separately
unlicensed extractor reference. The DOS executable is not needed, downloaded,
executed or redistributed by this project.

The local 60-level implementation remains intact. An explicit user-initiated
retrieval of the pinned files can restore build inputs in a source-only checkout;
integrity hashes do not confer redistribution rights. Building locally does not
make the bundled binary eligible for public release.

## DIFF EQ utility provenance

The user's existing DIFF EQ workspace was consulted read-only at development
commit `1a16b1b728c7c9e8a0e2491d96fde4b95cdacdbb`. Its source, history, outputs and
save namespace remain independent. A fresh audit found the public repository
[omegalpha210/fx-cg50-diffeq](https://github.com/omegalpha210/fx-cg50-diffeq) at
[`8e6f8f653f246427009beabbb8190f83bd2ab4df`](https://github.com/omegalpha210/fx-cg50-diffeq/tree/8e6f8f653f246427009beabbb8190f83bd2ab4df)
with an explicit [MIT license](https://github.com/omegalpha210/fx-cg50-diffeq/blob/8e6f8f653f246427009beabbb8190f83bd2ab4df/LICENSE),
copyright 2026 DIFFEQ contributors. The exact license is retained in
[DIFFEQ-LICENSE.txt](third_party/DIFFEQ-LICENSE.txt).

| SOKOBAN use | Public, MIT-licensed reference |
|---|---|
| `tools/verify_g3a.py` | [DIFF EQ package verifier](https://github.com/omegalpha210/fx-cg50-diffeq/blob/8e6f8f653f246427009beabbb8190f83bd2ab4df/tools/verify_g3a.py), specialized for SOKOBAN identity. |
| `tools/font_data.py` | Atlas extraction algorithm adapted from [DIFF EQ host font converter](https://github.com/omegalpha210/fx-cg50-diffeq/blob/8e6f8f653f246427009beabbb8190f83bd2ab4df/tools/host_font.py). The atlas has gint's separate terms below. |
| Build, UI and lifecycle conventions | Reference for fxSDK structure, softkeys, fresh-key handling, MENU/power lifecycle, OS world switching and transactional recovery. No numerical solver or private history is included. |

Both public utility files were compared byte-for-byte with their original local
DIFF EQ versions and matched. Their SHA-256 values are respectively
`ca715f1ff69ee2265b30bd6f0a12880c9986312641b7b472b87dac780205b3f7`
and `4a139c47e20cf91af99e635dcc4910f98aa10a4a1cb106176eb4cf1a28651c6a`.
This replaces the earlier provisional finding based only on the local development
root, which had no license. The preserved MIT notice supplies public license
evidence for the copied/adapted portions. The fxSDK notice is also retained for
the package-format reference.

## Font and original graphics

`assets/font/font8x9.png` is a byte-for-byte copy of `src/font8x9.png` from gint
revision
[`badbd0fd2bd8ac796fd55d49b93691741bd8a139`](https://git.planet-casio.com/Lephenixnoir/gint/src/commit/badbd0fd2bd8ac796fd55d49b93691741bd8a139).
Its SHA-256 is
`0e7f56f30e021e16053360b972adc0b020c7d8a2d545fbd883bc8e7d870413c8`.
`src/ui/font_data.h` is generated from that atlas for the shared app/host renderer.
The pinned [gint README](third_party/gint-README.md) permits use, sharing,
modification and sharing changes; credit is not required. The retained README
matches the pinned installed source exactly. No separate font restriction was
found there. No standard SPDX identifier is invented for this prose permission
statement, and no proprietary calculator font was extracted.

Wall/crate patterns, goal dots, player silhouettes, menu geometry and SOKOBAN
package icons are original project geometry. The DIFF EQ icon was inspected only
as a placement reference; its artwork was not copied into the SOKOBAN icon.

For v0.1.0-beta.2, `tools/make_icons.py` generates a new original warehouse scene
on one 10×10 pixel grid. No third-party sprite, copied commercial Sokoban icon,
or CASIO icon art is used. The 92×64 RGB assets and player/crate/target layout
are created in this project and covered by its MIT license. The prior project
icons are retained only for audit comparisons. DIFF EQ placement previews that
contain its actual artwork remain local in excluded `docs/captures`; public
icon audit output retains numeric reference measurements and SOKOBAN artwork.
The icon change supplies no new map-license evidence and changes none of the
existing map/bundled-binary redistribution restrictions.

Beta.4 README images use the actual application renderer with unchanged pinned
levels 1, 16, 31 and 59; restart/completion views use level 1. Completion comes
from replaying legal input from its original start. These selected images are
allowlisted under the owner's explicit request to show actual game puzzles,
replacing the former independently authored test-map illustrations. This does
not establish upstream redistribution permission or apply MIT to map geometry.
Other real-map captures remain excluded. Main and numbered level menus contain
project UI geometry. All images are labeled as host captures, not calculator
photographs or CPU-emulator captures. See [capture provenance](screenshots/README.md).
The original fixture remains a renderer test only. Palette-audit captions use
Pillow's bundled font; no standalone caption font is bundled.

## SDK and library inventory

The existing installation is reused without upgrade. Public build files use
environment variables or gitignored configuration, not private absolute paths.

| Component | Pinned installed revision | Retained evidence |
|---|---|---|
| [gint](https://git.planet-casio.com/Lephenixnoir/gint) | `badbd0fd2bd8ac796fd55d49b93691741bd8a139` | [README permission](third_party/gint-README.md); runtime and font. |
| [fxSDK](https://git.planet-casio.com/Lephenixnoir/fxsdk) | `09e2cf5fdab8d529217203a9a97a93facaa57311` | [MIT](third_party/fxSDK-LICENSE.txt); tools and package-format reference. |
| [FxLibc](https://git.planet-casio.com/Vhex-Kernel-Core/fxlibc) | `e2f458fa3d88d11b0685ae66d267730980eb65ca` | [CC0-1.0](third_party/fxlibc-LICENSE.txt), with separately licensed portions. |
| [OpenLibm](https://git.planet-casio.com/Lephenixnoir/OpenLibm) | `9828d2e3f265a882af6a5bba8ffe6a00c3eeb8d7` | [Combined notices](third_party/OpenLibm-LICENSE.md): MIT/BSD/ISC/FDLIBM and public-domain portions. |
| [SH GCC build scripts](https://git.planet-casio.com/Lephenixnoir/sh-elf-gcc) | `fdbc1be7a84e88f45f9b768b7428ff33b77eb9bb` | External installed GCC 14.1.0; no toolchain binary is distributed. |
| [SH binutils build scripts](https://git.planet-casio.com/Lephenixnoir/sh-elf-binutils) | `a963554d12c0236ee10c7545daa4c422bf107de6` | External assembler/linker; no toolchain binary is distributed. |

The [Grisu2b MIT notice](third_party/fxlibc-Grisu2b-LICENSE.txt) is retained. FxLibc
also contains separately licensed TinyMT sources; they are not copied into this
repository. Exact library object selection and all binary notices, including
libgcc's GPLv3 with GCC Runtime Library Exception, need a binary-distribution audit
if a distributable map pack is obtained later. This source-only candidate does not
distribute the map-containing binary or compiler/runtime archives. OpenLibm's
separate LGPL test programs are not project source. Notice copies and hashes are
indexed in [notice-manifest.json](third_party/notice-manifest.json).

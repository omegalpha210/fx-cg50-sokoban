# Third-party notices

The [MIT license](LICENSE), copyright 2026 omegalpha210, covers project-authored
SOKOBAN code, documentation, original graphics and original test fixtures. It
does not relicense the following third-party material. Exact revisions and source
hashes are recorded in [asset provenance](docs/ASSET_PROVENANCE.md).

| Material | Use and terms |
|---|---|
| [DIFF EQ](https://github.com/omegalpha210/fx-cg50-diffeq/tree/8e6f8f653f246427009beabbb8190f83bd2ab4df) | Copied package verifier and adapted font conversion algorithm. MIT, copyright 2026 DIFFEQ contributors. Preserve [the complete notice](docs/third_party/DIFFEQ-LICENSE.txt). |
| [gint](https://git.planet-casio.com/Lephenixnoir/gint) | Font atlas, generated font data, API-compatible host declarations and external native runtime. Custom permissive [README statement](docs/third_party/gint-README.md), with no standard SPDX identifier asserted. No proprietary CASIO font is bundled. |
| [fxSDK](https://git.planet-casio.com/Lephenixnoir/fxsdk) | External build/packaging tools and package-format reference. [MIT, gint/fxSDK contributors](docs/third_party/fxSDK-LICENSE.txt). |
| [FxLibc](https://git.planet-casio.com/Vhex-Kernel-Core/fxlibc) | External native C library. [CC0-1.0](docs/third_party/fxlibc-LICENSE.txt), except separate third-party portions; the [Grisu2b MIT notice](docs/third_party/fxlibc-Grisu2b-LICENSE.txt) is retained. |
| [OpenLibm SH port](https://git.planet-casio.com/Lephenixnoir/OpenLibm) | External native math library. [Combined upstream notices](docs/third_party/OpenLibm-LICENSE.md); individual component terms remain authoritative. |
| [begoon/sokoban-maps](https://github.com/begoon/sokoban-maps/tree/7aacdd777e383c172fd1879b31999748d12404f1) | Referenced 60-map input pack extracted from DOS `pusher`; extraction repository attribution: Alexander Demin. Redistribution permission is unconfirmed. The public-source candidate excludes these maps, upstream reference files, generated embedded data and bundled `.g3a` files. The owner requested selected actual-map README screenshots; their map geometry retains these separate rights, with no new upstream grant established. **The maps are not MIT-licensed by this project.** |

The atlas and font data retain gint's permission statement. SOKOBAN icons and game
geometry are project-created. Beta.4 README screenshots render original pack
levels 1/16/31/59 and level 1 restart/completion, under the owner's narrow preview
request. Their map layouts are third-party content, not project MIT artwork.
Main and level menus contain project UI geometry. [Capture provenance](docs/screenshots/README.md)
distinguishes host rendering from hardware photographs.

CMake, Python, Pillow, GCC and binutils are external development tools. Their
executables, SDK caches, source trees and toolchain archives are not part of this
source distribution. Pillow may supply its built-in font when making image
captions, but no standalone Pillow font is copied into the add-in. CASIO manuals,
PDFs and extracted manual images are excluded. CASIO trademarks do not imply
affiliation or endorsement.

A future binary release requires separate confirmation of map rights and review
of the exact linked objects and binary notices, including the GCC Runtime Library
Exception where applicable. Withholding a map-containing binary does not prevent
sharing the independently authored engine, UI, importer, tests or build scripts.

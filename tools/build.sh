#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
source tools/env.sh
"$SOKOBAN_PYTHON" tools/import_maps.py --check
fxsdk build-cg -j8
bash tools/package.sh

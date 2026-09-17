#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
source tools/env.sh
"$SOKOBAN_PYTHON" tools/import_maps.py --check
"$SOKOBAN_PYTHON" -m unittest discover -s tests -p 'test_*.py' -v
cmake -S tests -B build-host -DCMAKE_C_COMPILER=clang -DSOK_SANITIZE=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-host -j8
ctest --test-dir build-host --output-on-failure

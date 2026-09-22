#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
source tools/env.sh
mkdir -p dist
# Regenerate even if dist was deleted while the ELF remains up to date.
SOKOBAN_DATE_ARGS=()
if [[ -n "${SOURCE_DATE_EPOCH:-}" ]]; then
  SOKOBAN_PACKAGE_DATE="$("$SOKOBAN_PYTHON" -c 'import datetime,os; print(datetime.datetime.fromtimestamp(int(os.environ["SOURCE_DATE_EPOCH"]),datetime.timezone.utc).strftime("%Y.%m%d.%H%M"))')"
  SOKOBAN_DATE_ARGS=("--date=$SOKOBAN_PACKAGE_DATE")
fi
fxgxa --g3a "${SOKOBAN_DATE_ARGS[@]}" build-cg/sokoban.bin -o dist/SOKOBAN.g3a -n SOKOBAN \
  --internal=@SOKOBAN --version=00.01.0003 \
  --icon-uns=assets/icon-uns.png --icon-sel=assets/icon-sel.png
"$SOKOBAN_PYTHON" tools/verify_g3a.py dist/SOKOBAN.g3a
"$SOKOBAN_PYTHON" - <<'PY'
import hashlib
from pathlib import Path
p=Path('dist/SOKOBAN.g3a')
Path('dist/SHA256SUMS.txt').write_text(f'{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.name}\n')
PY

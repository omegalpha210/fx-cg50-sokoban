#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
source tools/env.sh
mkdir -p .local/validation docs/validation
bash tools/build.sh 2>&1 | tee .local/validation/build.log
"$SOKOBAN_PYTHON" tools/layout_audit.py > .local/validation/layout-summary.json
"$SOKOBAN_PYTHON" tools/icon_audit.py --check > .local/validation/icon-summary.json
bash tools/test.sh 2>&1 | tee .local/validation/tests.log
"$SOKOBAN_PYTHON" tools/captures.py
"$SOKOBAN_PYTHON" tools/public_captures.py
"$SOKOBAN_PYTHON" tools/layout_audit.py --check --capture-csv build-host/captures/layout.csv > .local/validation/layout-summary.json
"$SOKOBAN_PYTHON" tools/memory_report.py
fxgxa -d dist/SOKOBAN.g3a > .local/validation/fxgxa-dump.txt
{
  build-host/test_engine
  build-host/test_storage
  build-host/test_workflow
  build-host/test_storage_native
  build-host/test_power
  build-host/test_idle
  build-host/test_system_power
  build-host/capture_public build-host/public-captures
  build-host/capture build-host/captures
  "$SOKOBAN_PYTHON" tools/verify_g3a.py dist/SOKOBAN.g3a
} | tee docs/validation/results.txt
"$SOKOBAN_PYTHON" - <<'PY'
import hashlib
from pathlib import Path
root=Path.cwd()
for name in ('build','tests'):
    text=(root/f'.local/validation/{name}.log').read_text()
    # Keep reviewable evidence without baking a private checkout path into it.
    (root/f'docs/validation/{name}.log').write_text(text.replace(str(root),'$PROJECT'))
(root/'docs/validation/fxgxa-dump.txt').write_text('\n'.join(x.rstrip() for x in (root/'.local/validation/fxgxa-dump.txt').read_text().splitlines()).rstrip()+'\n')
p=Path('dist/SOKOBAN.g3a')
assert Path('dist/SHA256SUMS.txt').read_text()==f'{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.name}\n'
print('Final artifact matches SHA256SUMS.txt.')
PY

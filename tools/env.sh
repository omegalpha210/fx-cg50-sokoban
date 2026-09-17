#!/usr/bin/env bash
# Optional machine-specific setup is deliberately untracked.
if [[ -n "${BASH_VERSION:-}" ]]; then
  SOKOBAN_ENV_FILE="${BASH_SOURCE[0]}"
elif [[ -n "${ZSH_VERSION:-}" ]]; then
  SOKOBAN_ENV_FILE="${(%):-%x}"
else
  echo 'Source tools/env.sh with bash or zsh.' >&2
  return 1
fi
SOKOBAN_ROOT="$(cd "$(dirname "$SOKOBAN_ENV_FILE")/.." && pwd)"
if [[ -f "$SOKOBAN_ROOT/.local/env.sh" ]]; then source "$SOKOBAN_ROOT/.local/env.sh"; fi
if [[ -n "${SOKOBAN_SDK_ROOT:-}" ]]; then
  export PATH="$SOKOBAN_SDK_ROOT/prefix/bin:$SOKOBAN_SDK_ROOT/prefix/share/fxsdk/sysroot/bin:$SOKOBAN_SDK_ROOT/venv/bin:$PATH"
fi
export SOKOBAN_PYTHON="${SOKOBAN_PYTHON:-python3}"

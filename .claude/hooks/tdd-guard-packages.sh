#!/usr/bin/env bash
# Path-scoped TDD-Guard wrapper for sprig-ao.
#
# Forwards Edit/Write/MultiEdit hook input to tdd-guard ONLY when the target
# file is under packages/*/src/ (production code). Edits to docs/, scripts/,
# config files, tests, .orchestrator/, etc. pass through untouched (exit 0).
#
# Claude Code passes hook input as JSON on stdin:
#   { "tool_name": "...", "tool_input": { "file_path": "..." }, ... }
#
# Note: tdd-guard is installed under the user-local npm prefix
# (~/.npm-global/bin) so it works regardless of /usr/local permissions.
set -euo pipefail

export PATH="$HOME/.npm-global/bin:$PATH"

input=$(cat)

if command -v jq >/dev/null 2>&1; then
  target=$(printf '%s' "$input" | jq -r '.tool_input.file_path // ""' 2>/dev/null || printf '')
else
  target=$(printf '%s' "$input" | python3 -c 'import json,sys
try:
    data = json.load(sys.stdin)
except Exception:
    sys.exit(0)
ti = data.get("tool_input") or {}
print(ti.get("file_path") or "")
' 2>/dev/null || printf '')
fi

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

# Normalize to repo-relative.
case "$target" in
  /*)
    if [[ "$target" == "$repo_root"/* ]]; then
      rel="${target#$repo_root/}"
    else
      rel="$target"
    fi
    ;;
  *)
    rel="$target"
    ;;
esac

# Match packages/*/src/**, but NOT packages/*/src/**/__tests__/** or *.test.ts(x).
case "$rel" in
  packages/*/src/*__tests__*|packages/*/src/*.test.ts|packages/*/src/*.test.tsx|packages/*/src/*.spec.ts|packages/*/src/*.spec.tsx)
    exit 0
    ;;
  packages/*/src/*)
    printf '%s' "$input" | exec tdd-guard
    ;;
  *)
    exit 0
    ;;
esac

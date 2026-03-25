#!/usr/bin/env bash
# Smoke tests for shell scripts (dry-run mode).
# Run: bash tools/tests/test_shell_scripts.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

PASS=0
FAIL=0

pass() { echo "  ✓ $1"; PASS=$((PASS+1)); }
fail() { echo "  ✗ $1"; FAIL=$((FAIL+1)); }

echo "=== Shell script smoke tests ==="

# --- clean.sh --dry-run ---
echo ""
echo "Testing clean.sh --dry-run..."

output=$("$REPO_ROOT/tools/clean.sh" --dry-run 2>&1) && rc=0 || rc=$?
if [[ $rc -eq 0 ]]; then
    pass "clean.sh --dry-run exits 0"
else
    fail "clean.sh --dry-run exited $rc"
fi

if echo "$output" | grep -qi "dry.run\|would\|skip"; then
    pass "clean.sh --dry-run produces dry-run output"
else
    fail "clean.sh --dry-run: no dry-run markers in output"
fi

# --- clean.sh --all --dry-run ---
output=$("$REPO_ROOT/tools/clean.sh" --all --dry-run 2>&1) && rc=0 || rc=$?
if [[ $rc -eq 0 ]]; then
    pass "clean.sh --all --dry-run exits 0"
else
    fail "clean.sh --all --dry-run exited $rc"
fi

# --- clean.sh --help ---
output=$("$REPO_ROOT/tools/clean.sh" --help 2>&1) && rc=0 || rc=$?
if [[ $rc -eq 0 ]]; then
    pass "clean.sh --help exits 0"
else
    fail "clean.sh --help exited $rc"
fi

# --- Summary ---
echo ""
echo "Results: $PASS passed, $FAIL failed"
[[ $FAIL -eq 0 ]]

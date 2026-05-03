#!/bin/bash
# Update Qt and JSON translation files
# Run from repository root: ./tools/translations/qgc-lupdate.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Use QT_ROOT_DIR if set (CI), otherwise auto-detect via read_config.py
if [[ -n "${QT_ROOT_DIR:-}" ]]; then
    LUPDATE="${QT_ROOT_DIR}/bin/lupdate"
else
    QT_VERSION=$(python3 "$REPO_ROOT/tools/setup/read_config.py" --get qt_version 2>/dev/null || echo "")
    if [[ -n "$QT_VERSION" ]]; then
        # Search standard Qt install locations
        for base in "$HOME/Qt" "/opt/Qt"; do
            for arch in gcc_64 clang_64 linux_gcc_64; do
                candidate="$base/$QT_VERSION/$arch/bin/lupdate"
                if [[ -x "$candidate" ]]; then
                    LUPDATE="$candidate"
                    break 2
                fi
            done
        done
    fi
    LUPDATE="${LUPDATE:-lupdate}"
fi

if [[ ! -x "$LUPDATE" ]] && ! command -v "$LUPDATE" &>/dev/null; then
    echo "Error: lupdate not found at $LUPDATE" >&2
    echo "Set QT_ROOT_DIR or install Qt tools" >&2
    exit 1
fi

echo "Using lupdate: $LUPDATE"
LUPDATE_OUTPUT=$("$LUPDATE" src -ts translations/qgc.ts -no-obsolete 2>&1) || LUPDATE_EXIT=$?
LUPDATE_EXIT=${LUPDATE_EXIT:-0}
echo "$LUPDATE_OUTPUT"
if [[ $LUPDATE_EXIT -ne 0 ]]; then
    echo "Error: lupdate failed with exit code $LUPDATE_EXIT" >&2
    exit 1
fi
FAILED=0

# Check for tr() without context (file-scope or anonymous-namespace tr() calls)
NO_CONTEXT=$(echo "$LUPDATE_OUTPUT" | grep "tr() cannot be called without context" || true)
if [[ -n "$NO_CONTEXT" ]]; then
    echo "" >&2
    echo "Error: tr() called without a translation context in the following locations:" >&2
    echo "$NO_CONTEXT" >&2
    echo "" >&2
    echo "Fix: Replace QT_TR_NOOP(\"...\") with QT_TRANSLATE_NOOP(\"ClassName\", \"...\")" >&2
    echo "     Replace tr(\"...\") with QCoreApplication::translate(\"ClassName\", \"...\")" >&2
    echo "     (Use the enclosing class or namespace name as the context string)" >&2
    FAILED=1
fi

# Check for classes that use tr() but are missing Q_OBJECT
NO_QOBJECT=$(echo "$LUPDATE_OUTPUT" | grep "lacks Q_OBJECT macro" || true)
if [[ -n "$NO_QOBJECT" ]]; then
    echo "" >&2
    echo "Error: Class uses tr() but is missing Q_OBJECT macro:" >&2
    echo "$NO_QOBJECT" >&2
    echo "" >&2
    echo "Fix: Add Q_OBJECT to the class declaration in the .h file," >&2
    echo "     or replace tr(\"...\") with QCoreApplication::translate(\"ClassName\", \"...\")" >&2
    FAILED=1
fi

if [[ $FAILED -ne 0 ]]; then
    exit 1
fi

echo "Extracting JSON strings..."
python3 tools/translations/qgc-lupdate-json.py

echo "Generating pseudo-localization files..."
python3 tools/pseudo_loc.py

echo "Translation files updated"

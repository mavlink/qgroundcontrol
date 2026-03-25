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
"$LUPDATE" src -ts translations/qgc.ts -no-obsolete

echo "Extracting JSON strings..."
python3 tools/translations/qgc-lupdate-json.py

echo "Translation files updated"

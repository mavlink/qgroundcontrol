#!/bin/bash
# Update Qt and JSON translation files
# Run from repository root: ./tools/translations/qgc-lupdate.sh

set -euo pipefail

# Use QT_ROOT_DIR if set (CI), otherwise find local Qt installation
if [[ -n "${QT_ROOT_DIR:-}" ]]; then
    LUPDATE="${QT_ROOT_DIR}/bin/lupdate"
else
    QT_PATH=(~/Qt/6.1[0-9].*/*/bin)
    LUPDATE="${QT_PATH[0]}/lupdate"
fi

if [[ ! -x "$LUPDATE" ]]; then
    echo "Error: lupdate not found at $LUPDATE" >&2
    exit 1
fi

echo "Using lupdate: $LUPDATE"
"$LUPDATE" src -ts translations/qgc.ts -no-obsolete

echo "Extracting JSON strings..."
python3 tools/translations/qgc-lupdate-json.py

echo "Translation files updated"

#!/bin/bash
# This script will update both the Qt and Json string translation files.
# Run from repository root: source tools/translations/qgc-lupdate.sh
#
# The script looks for lupdate in the following order:
# 1. QT_ROOT_DIR environment variable (e.g., from aqtinstall)
# 2. Common Qt installation paths
# 3. System PATH

set -e

find_lupdate() {
    # Check QT_ROOT_DIR first
    if [[ -n "$QT_ROOT_DIR" && -x "$QT_ROOT_DIR/bin/lupdate" ]]; then
        echo "$QT_ROOT_DIR/bin/lupdate"
        return 0
    fi

    # Check common Qt installation paths
    local qt_paths=(
        ~/Qt/6.1[0-9].*/*/bin/lupdate
        ~/Qt/6.*/*/bin/lupdate
        /opt/Qt/6.*/*/bin/lupdate
        /usr/lib/qt6/bin/lupdate
    )

    for pattern in "${qt_paths[@]}"; do
        # shellcheck disable=SC2206
        local matches=($pattern)
        if [[ -x "${matches[0]}" ]]; then
            echo "${matches[0]}"
            return 0
        fi
    done

    # Fall back to PATH
    if command -v lupdate &> /dev/null; then
        command -v lupdate
        return 0
    fi

    return 1
}

LUPDATE=$(find_lupdate) || {
    echo "Error: Could not find lupdate. Set QT_ROOT_DIR or install Qt." >&2
    exit 1
}

echo "Using lupdate: $LUPDATE"
"$LUPDATE" src -ts translations/qgc.ts -no-obsolete
python3 tools/translations/qgc-lupdate-json.py

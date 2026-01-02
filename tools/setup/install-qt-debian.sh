#!/usr/bin/env bash
#
# Install Qt on Linux via aqtinstall
#
# DEPRECATION NOTICE: This script is a thin wrapper around install-qt.py.
# For new development, use install-qt.py directly:
#   python3 tools/setup/install-qt.py --host linux --target desktop
#
# Environment variables (all optional, defaults from build-config.json):
#   QT_VERSION      - Qt version to install
#   QT_PATH         - Installation prefix (default: /opt/Qt)
#   QT_HOST         - Host platform (default: linux)
#   QT_TARGET       - Target platform (default: desktop, or: android, wasm)
#   QT_ARCH         - Architecture (default: linux_gcc_64)
#   QT_TOOLS        - Space-separated tools to install (e.g., "tools_ifw")
#   QT_MODULES      - Space-separated Qt modules to install
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PYTHON_SCRIPT="$SCRIPT_DIR/install-qt.py"

# Ensure Python 3 is available
if ! command -v python3 &> /dev/null; then
    echo "Installing Python 3..."
    if command -v apt-get &> /dev/null; then
        apt-get update -y --quiet
        apt-get install -y python3 python3-pip
    elif command -v dnf &> /dev/null; then
        dnf install -y python3 python3-pip
    else
        echo "Error: Python 3 required but could not install" >&2
        exit 1
    fi
fi

# Build arguments for Python script
ARGS=()
ARGS+=(--host "${QT_HOST:-linux}")
ARGS+=(--target "${QT_TARGET:-desktop}")
ARGS+=(--install-aqt)

[[ -n "${QT_VERSION:-}" ]] && ARGS+=(--version "$QT_VERSION")
[[ -n "${QT_PATH:-}" ]] && ARGS+=(--path "$QT_PATH")
[[ -n "${QT_ARCH:-}" ]] && ARGS+=(--arch "$QT_ARCH")
[[ -n "${QT_MODULES:-}" ]] && ARGS+=(--modules "$QT_MODULES")
[[ -n "${QT_TOOLS:-}" ]] && ARGS+=(--tools "$QT_TOOLS")

# Run Python script
exec python3 "$PYTHON_SCRIPT" "${ARGS[@]}"

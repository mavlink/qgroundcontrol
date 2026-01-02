#!/usr/bin/env bash
#
# Install Qt for iOS development on macOS
#
# DEPRECATION NOTICE: This script is a thin wrapper around install-qt.py.
# For new development, use install-qt.py directly:
#   python3 tools/setup/install-qt.py --target ios
#
# This script installs both host (macOS) Qt and iOS target Qt, which are
# required for cross-compiling Qt applications for iOS.
#
# Prerequisites:
#   - macOS with Xcode installed (xcode-select --install)
#
# Environment variables (all optional, defaults from build-config.json):
#   QT_VERSION      - Qt version to install
#   QT_PATH         - Installation prefix (default: /opt/Qt)
#   QT_MODULES      - Space-separated Qt modules for host (from build-config.json)
#   QT_MODULES_IOS  - Space-separated Qt modules for iOS (defaults to QT_MODULES)
#
# Usage:
#   ./install-qt-ios.sh
#
# After installation, source the environment:
#   export QT_ROOT_DIR=/opt/Qt/<version>/ios
#   export QT_HOST_PATH=/opt/Qt/<version>/macos
#

set -euo pipefail

# Ensure we're on macOS
if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "Error: This script only runs on macOS" >&2
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PYTHON_SCRIPT="$SCRIPT_DIR/install-qt.py"

# Ensure Python 3 is available
if ! command -v python3 &> /dev/null; then
    echo "Installing Python 3 via Homebrew..."
    brew install python3
fi

# Build arguments for Python script
ARGS=()
ARGS+=(--target ios)
ARGS+=(--host mac)
ARGS+=(--install-aqt)

[[ -n "${QT_VERSION:-}" ]] && ARGS+=(--version "$QT_VERSION")
[[ -n "${QT_PATH:-}" ]] && ARGS+=(--path "$QT_PATH")
[[ -n "${QT_MODULES:-}" ]] && ARGS+=(--modules "$QT_MODULES")
[[ -n "${QT_MODULES_IOS:-}" ]] && ARGS+=(--ios-modules "$QT_MODULES_IOS")

# Run Python script
exec python3 "$PYTHON_SCRIPT" "${ARGS[@]}"

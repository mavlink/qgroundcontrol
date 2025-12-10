#!/usr/bin/env bash

set -e

# Source build config (required - no fallbacks)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ -f "$SCRIPT_DIR/read-config.sh" ]]; then
    source "$SCRIPT_DIR/read-config.sh"
else
    echo "Error: read-config.sh not found" >&2
    exit 1
fi

# Verify required variables are set
if [[ -z "${QT_VERSION:-}" ]] || [[ -z "${QT_MODULES:-}" ]]; then
    echo "Error: QT_VERSION and QT_MODULES must be set (check build-config.json)" >&2
    exit 1
fi

# Platform defaults (can be overridden via environment)
QT_PATH="${QT_PATH:-/opt/Qt}"
QT_HOST="${QT_HOST:-mac}"
QT_TARGET="${QT_TARGET:-desktop}"
QT_ARCH="${QT_ARCH:-mac}"
IFS=' ' read -r -a QT_MODULES_ARR <<< "$QT_MODULES"

echo "QT_VERSION: $QT_VERSION"
echo "QT_PATH:    $QT_PATH"
echo "QT_HOST:    $QT_HOST"
echo "QT_TARGET:  $QT_TARGET"
echo "QT_ARCH:    $QT_ARCH"
echo "QT_MODULES: ${QT_MODULES_ARR[*]}"

# Update Homebrew and install Python 3 (if needed)
brew update
brew install python3

# Install required Python packages, including aqtinstall.
pip3 install --upgrade setuptools wheel py7zr ninja cmake aqtinstall

# Use aqtinstall to download and install Qt.
aqt install-qt "${QT_HOST}" "${QT_TARGET}" "${QT_VERSION}" "${QT_ARCH}" -O "${QT_PATH}" -m "${QT_MODULES_ARR[@]}"

# macOS does not support GNU readlink -e.
# We use realpath instead (or substitute with greadlink -f if needed).
# Note: On macOS, dynamic libraries are resolved using DYLD_LIBRARY_PATH.
QT_BIN_DIR=$(realpath "${QT_PATH}/${QT_VERSION}/${QT_ARCH}/bin")
export PATH="$QT_BIN_DIR:$PATH"
QT_PKGCONFIG_DIR=$(realpath "${QT_PATH}/${QT_VERSION}/${QT_ARCH}/lib/pkgconfig")
export PKG_CONFIG_PATH="$QT_PKGCONFIG_DIR:${PKG_CONFIG_PATH:-}"
QT_LIB_DIR=$(realpath "${QT_PATH}/${QT_VERSION}/${QT_ARCH}/lib")
export DYLD_LIBRARY_PATH="$QT_LIB_DIR:${DYLD_LIBRARY_PATH:-}"
QT_ROOT_DIR=$(realpath "${QT_PATH}/${QT_VERSION}/${QT_ARCH}")
export QT_ROOT_DIR
QT_PLUGIN_PATH=$(realpath "${QT_PATH}/${QT_VERSION}/${QT_ARCH}/plugins")
export QT_PLUGIN_PATH
QML2_IMPORT_PATH=$(realpath "${QT_PATH}/${QT_VERSION}/${QT_ARCH}/qml")
export QML2_IMPORT_PATH

echo "Updated environment variables:"
echo "PATH:              $PATH"
echo "PKG_CONFIG_PATH:   $PKG_CONFIG_PATH"
echo "DYLD_LIBRARY_PATH: $DYLD_LIBRARY_PATH"
echo "QT_ROOT_DIR:       $QT_ROOT_DIR"
echo "QT_PLUGIN_PATH:    $QT_PLUGIN_PATH"
echo "QML2_IMPORT_PATH:  $QML2_IMPORT_PATH"

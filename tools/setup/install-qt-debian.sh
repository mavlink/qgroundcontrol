#!/usr/bin/env bash
set -euo pipefail

# Source build config (required - no fallbacks)
SCRIPT_DIR="$(dirname "${BASH_SOURCE[0]}")/read-config.sh"
if [[ -f "$SCRIPT_DIR" ]]; then
    . "${SCRIPT_DIR}"
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
QT_HOST="${QT_HOST:-linux}"
QT_TARGET="${QT_TARGET:-desktop}"
QT_ARCH="${QT_ARCH:-linux_gcc_64}"
QT_ARCH_DIR="${QT_ARCH_DIR:-gcc_64}"
QT_ROOT_DIR="${QT_ROOT_DIR:-${QT_PATH}/${QT_VERSION}/${QT_ARCH_DIR}}"
IFS=' ' read -r -a QT_MODULES_ARR <<< "$QT_MODULES"

echo "QT_VERSION $QT_VERSION"
echo "QT_PATH $QT_PATH"
echo "QT_HOST $QT_HOST"
echo "QT_TARGET $QT_TARGET"
echo "QT_ARCH $QT_ARCH"
echo "QT_ARCH_DIR $QT_ARCH_DIR"
echo "QT_ROOT_DIR $QT_ROOT_DIR"
echo "QT_MODULES ${QT_MODULES_ARR[*]}"

apt-get update -qq
apt-get install -qq python3 python3-pip pipx
pipx run aqtinstall install-qt "${QT_HOST}" "${QT_TARGET}" "${QT_VERSION}" "${QT_ARCH}" -O "${QT_PATH}" -m "${QT_MODULES_ARR[@]}"

QT_ROOT_DIR=$(readlink -e "${QT_ROOT_DIR}")
export QT_ROOT_DIR
PATH=$(readlink -e "${QT_ROOT_DIR}/bin/"):$PATH
export PATH
PKG_CONFIG_PATH=$(readlink -e "${QT_ROOT_DIR}/lib/pkgconfig"):${PKG_CONFIG_PATH:-}
export PKG_CONFIG_PATH
LD_LIBRARY_PATH=$(readlink -e "${QT_ROOT_DIR}/lib"):${LD_LIBRARY_PATH:-}
export LD_LIBRARY_PATH
QT_PLUGIN_PATH=$(readlink -e "${QT_ROOT_DIR}/plugins")
export QT_PLUGIN_PATH
QML2_IMPORT_PATH=$(readlink -e "${QT_ROOT_DIR}/qml")
export QML2_IMPORT_PATH

echo "QT_ROOT_DIR $QT_ROOT_DIR"
echo "PATH $PATH"
echo "PKG_CONFIG_PATH $PKG_CONFIG_PATH"
echo "LD_LIBRARY_PATH $LD_LIBRARY_PATH"
echo "QT_PLUGIN_PATH $QT_PLUGIN_PATH"
echo "QML2_IMPORT_PATH $QML2_IMPORT_PATH"

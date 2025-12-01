#!/usr/bin/env bash
#
# Install Qt on Debian/Ubuntu via aqtinstall

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Source centralized config (sets QT_VERSION, QT_MODULES if not already set)
# shellcheck source=read-config.sh
source "$SCRIPT_DIR/read-config.sh"

QT_PATH="${QT_PATH:-/opt/Qt}"
QT_HOST="${QT_HOST:-linux}"
QT_TARGET="${QT_TARGET:-desktop}"
QT_ARCH="${QT_ARCH:-linux_gcc_64}"
QT_ARCH_DIR="${QT_ARCH_DIR:-gcc_64}"
QT_ROOT_DIR="${QT_ROOT_DIR:-${QT_PATH}/${QT_VERSION}/${QT_ARCH_DIR}}"
QT_MODULES_STR="${QT_MODULES}"
IFS=' ' read -r -a QT_MODULES_ARR <<< "$QT_MODULES_STR"

echo "QT_VERSION $QT_VERSION"
echo "QT_PATH $QT_PATH"
echo "QT_HOST $QT_HOST"
echo "QT_TARGET $QT_TARGET"
echo "QT_ARCH $QT_ARCH"
echo "QT_ARCH_DIR $QT_ARCH_DIR"
echo "QT_ROOT_DIR $QT_ROOT_DIR"
echo "QT_MODULES ${QT_MODULES_ARR[*]}"

apt-get update -y --quiet
apt-get install python3 python3-pip pipx -y
pipx install aqtinstall
pipx install cmake
pipx install ninja
pipx ensurepath
USER_BASE_BIN="$(python3 -m site --user-base)/bin"
export PATH="$USER_BASE_BIN:$PATH"
aqt install-qt "${QT_HOST}" "${QT_TARGET}" "${QT_VERSION}" "${QT_ARCH}" -O "${QT_PATH}" -m "${QT_MODULES_ARR[@]}"

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

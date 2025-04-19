#!/usr/bin/env bash

# Set defaults appropriate for macOS.
QT_VERSION="${QT_VERSION:-6.8.3}"
QT_PATH="${QT_PATH:-/opt/Qt}"
QT_HOST="${QT_HOST:-mac}"
QT_TARGET="${QT_TARGET:-desktop}"
QT_ARCH="${QT_ARCH:-mac}"
QT_MODULES="${QT_MODULES:-qtcharts qtlocation qtpositioning qtspeech qt5compat qtmultimedia qtserialport qtimageformats qtshadertools qtconnectivity qtquick3d qtsensors}"

set -e

echo "QT_VERSION: $QT_VERSION"
echo "QT_PATH:    $QT_PATH"
echo "QT_HOST:    $QT_HOST"
echo "QT_TARGET:  $QT_TARGET"
echo "QT_ARCH:    $QT_ARCH"
echo "QT_MODULES: $QT_MODULES"

# Update Homebrew and install Python 3 (if needed)
brew update
brew install python3

# Install required Python packages, including aqtinstall.
pip3 install --upgrade setuptools wheel py7zr ninja cmake aqtinstall

# Use aqtinstall to download and install Qt.
aqt install-qt ${QT_HOST} ${QT_TARGET} ${QT_VERSION} ${QT_ARCH} -O ${QT_PATH} -m ${QT_MODULES}

# macOS does not support GNU readlink -e.
# We use realpath instead (or substitute with greadlink -f if needed).
# Note: On macOS, dynamic libraries are resolved using DYLD_LIBRARY_PATH.
export PATH="$(realpath ${QT_PATH}/${QT_VERSION}/${QT_ARCH}/bin):$PATH"
export PKG_CONFIG_PATH="$(realpath ${QT_PATH}/${QT_VERSION}/${QT_ARCH}/lib/pkgconfig):$PKG_CONFIG_PATH"
export DYLD_LIBRARY_PATH="$(realpath ${QT_PATH}/${QT_VERSION}/${QT_ARCH}/lib):$DYLD_LIBRARY_PATH"
export QT_ROOT_DIR="$(realpath ${QT_PATH}/${QT_VERSION}/${QT_ARCH})"
export QT_PLUGIN_PATH="$(realpath ${QT_PATH}/${QT_VERSION}/${QT_ARCH}/plugins)"
export QML2_IMPORT_PATH="$(realpath ${QT_PATH}/${QT_VERSION}/${QT_ARCH}/qml)"

echo "Updated environment variables:"
echo "PATH:              $PATH"
echo "PKG_CONFIG_PATH:   $PKG_CONFIG_PATH"
echo "DYLD_LIBRARY_PATH: $DYLD_LIBRARY_PATH"
echo "QT_ROOT_DIR:       $QT_ROOT_DIR"
echo "QT_PLUGIN_PATH:    $QT_PLUGIN_PATH"
echo "QML2_IMPORT_PATH:  $QML2_IMPORT_PATH"

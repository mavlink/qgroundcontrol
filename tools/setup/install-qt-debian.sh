#!/usr/bin/env bash

QT_VERSION="${QT_VERSION:-6.8.3}"
QT_PATH="${QT_PATH:-/opt/Qt}"
QT_HOST="${QT_HOST:-linux}"
QT_TARGET="${QT_TARGET:-desktop}"
QT_ARCH="${QT_ARCH:-linux_gcc_64}"
QT_MODULES="${QT_MODULES:-qtcharts qtlocation qtpositioning qtspeech qt5compat qtmultimedia qtserialport qtimageformats qtshadertools qtconnectivity qtquick3d qtsensors}"

set -e

echo "QT_VERSION $QT_VERSION"
echo "QT_PATH $QT_PATH"
echo "QT_HOST $QT_HOST"
echo "QT_TARGET $QT_TARGET"
echo "QT_ARCH $QT_ARCH"
echo "QT_MODULES $QT_MODULES"

apt update
apt install python3 python3-pip -y
pip3 install setuptools wheel py7zr ninja cmake aqtinstall
aqt install-qt ${QT_HOST} ${QT_TARGET} ${QT_VERSION} ${QT_ARCH} -O ${QT_PATH} -m ${QT_MODULES}
export PATH=$(readlink -e ${QT_PATH}/${QT_VERSION}/${QT_ARCH}/bin/):$PATH
export PKG_CONFIG_PATH=$(readlink -e ${QT_PATH}/${QT_VERSION}/${QT_ARCH}/lib/pkgconfig):$PKG_CONFIG_PATH
export LD_LIBRARY_PATH=$(readlink -e ${QT_PATH}/${QT_VERSION}/${QT_ARCH}/lib):$LD_LIBRARY_PATH
export QT_ROOT_DIR=$(readlink -e ${QT_PATH}/${QT_VERSION}/${QT_ARCH})
export QT_PLUGIN_PATH=$(readlink -e ${QT_PATH}/${QT_VERSION}/${QT_ARCH}/plugins)
export QML2_IMPORT_PATH=$(readlink -e ${QT_PATH}/${QT_VERSION}/${QT_ARCH}/qml)

echo "PATH $PATH"
echo "PKG_CONFIG_PATH $PKG_CONFIG_PATH"
echo "LD_LIBRARY_PATH $LD_LIBRARY_PATH"
echo "QT_ROOT_DIR $QT_ROOT_DIR"
echo "QT_PLUGIN_PATH $QT_PLUGIN_PATH"
echo "QML2_IMPORT_PATH $QML2_IMPORT_PATH"

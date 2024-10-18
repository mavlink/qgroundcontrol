#!/usr/bin/env bash

QT_VERSION="${QT_VERSION:-6.6.3}"
QT_PATH="${QT_PATH:-/opt/Qt}"
QT_HOST="${QT_HOST:-linux}"
QT_HOST_ARCH="${QT_HOST_ARCH:-gcc_64}"
QT_TARGET="${QT_TARGET:-android}"
QT_TARGET_ARCH="${QT_TARGET_ARCH:-android_arm64_v8a}"
QT_MODULES="${QT_MODULES:-qtcharts qtlocation qtpositioning qtspeech qt5compat qtmultimedia qtserialport qtimageformats qtshadertools qtconnectivity qtquick3d qtsensors}"

set -e

echo "QT_VERSION $QT_VERSION"
echo "QT_PATH $QT_PATH"
echo "QT_HOST $QT_HOST"
echo "QT_TARGET $QT_TARGET"
echo "QT_TARGET_ARCH $QT_TARGET_ARCH"
echo "QT_MODULES $QT_MODULES"

wget2 https://dl.google.com/android/repository/commandlinetools-linux-8512546_latest.zip
unzip commandlinetools-linux-8512546_latest.zip
mkdir -p /opt/Android/Sdk/cmdline-tools/latest/
mv cmdline-tools/* /opt/Android/Sdk/cmdline-tools/latest/
/opt/Android/Sdk/cmdline-tools/latest/bin/sdkmanager "ndk;25.1.8937393"
export ANDROID_NDK_HOME=/opt/Android/Sdk/ndk/25.1.8937393
export PATH=$PATH:$ANDROID_NDK_HOME

apt update
apt install python3 python3-pip -y
pip3 install setuptools wheel py7zr ninja cmake aqtinstall
aqt install-qt ${QT_HOST} desktop ${QT_VERSION} ${QT_HOST_ARCH} -O ${QT_PATH} -m ${QT_MODULES}
aqt install-qt ${QT_HOST} ${QT_TARGET} ${QT_VERSION} ${QT_TARGET_ARCH} -O ${QT_PATH} -m ${QT_MODULES} --autodesktop

export QT_ROOT_DIR=$(readlink -e ${QT_PATH}/${QT_VERSION}/${QT_TARGET_ARCH})
export QT_HOST_PATH=$(readlink -e ${QT_PATH}/${QT_VERSION}/${QT_HOST_ARCH})
export QT_PLUGIN_PATH=$(readlink -e ${QT_ROOT_DIR}/plugins)
export QML2_IMPORT_PATH=$(readlink -e ${QT_ROOT_DIR}/qml)
export PATH=$(readlink -e ${QT_ROOT_DIR}/bin/):$PATH
export PKG_CONFIG_PATH=$(readlink -e ${QT_ROOT_DIR}/lib/pkgconfig):$PKG_CONFIG_PATH
export LD_LIBRARY_PATH=$(readlink -e ${QT_ROOT_DIR}/lib):$LD_LIBRARY_PATH

echo "PATH $PATH"
echo "PKG_CONFIG_PATH $PKG_CONFIG_PATH"
echo "LD_LIBRARY_PATH $LD_LIBRARY_PATH"
echo "QT_ROOT_DIR $QT_ROOT_DIR"
echo "QT_HOST_PATH $QT_HOST_PATH"
echo "QT_PLUGIN_PATH $QT_PLUGIN_PATH"
echo "QML2_IMPORT_PATH $QML2_IMPORT_PATH"

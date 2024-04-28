#!/bin/bash -l

set -e
set -x

# apt update -y --quiet

# #Build Tools
# DEBIAN_FRONTEND=noninteractive apt -y --quiet install \
# 	appstream \
# 	binutils \
# 	build-essential \
# 	ccache \
# 	cmake \
# 	cppcheck \
# 	file \
# 	g++ \
# 	gcc \
# 	gdb \
# 	git \
# 	libfuse2 \
# 	libtool \
# 	locales \
# 	make \
# 	ninja-build \
# 	patchelf \
# 	pkgconf \
# 	python3 \
# 	python3-pip \
# 	rsync

# #Qt Required
# DEBIAN_FRONTEND=noninteractive apt -y --quiet install \
# 	libdrm-dev \
# 	libgl1-mesa-dev \
# 	libpulse-dev \
# 	libxcb-cursor0 \
# 	libxcb-glx0 \
# 	libxcb-icccm4 \
# 	libxcb-image0 \
# 	libxcb-keysyms1 \
# 	libxcb-randr0 \
# 	libxcb-render-util0 \
# 	libxcb-render0 \
# 	libxcb-shape0 \
# 	libxcb-shm0 \
# 	libxcb-sync1 \
# 	libxcb-util1 \
# 	libxcb-xfixes0 \
# 	libxcb-xinerama0 \
# 	libxcb-xkb-dev \
# 	libxcb1 \
# 	libxkbcommon-dev \
# 	libxkbcommon-x11-0 \
# 	libzstd-dev

# #QGC
# DEBIAN_FRONTEND=noninteractive apt -y --quiet install \
# 	flite \
# 	libsdl2-dev \
# 	libspeechd2 \
# 	speech-dispatcher \
# 	speech-dispatcher-flite

# #GStreamer
# DEBIAN_FRONTEND=noninteractive apt -y --quiet install \
# 	gstreamer1.0-alsa \
# 	gstreamer1.0-gl \
# 	gstreamer1.0-gl \
# 	gstreamer1.0-gtk3 \
# 	gstreamer1.0-libav \
# 	gstreamer1.0-plugins-bad \
# 	gstreamer1.0-plugins-base \
# 	gstreamer1.0-plugins-good \
# 	gstreamer1.0-plugins-rtp \
# 	gstreamer1.0-plugins-ugly \
# 	gstreamer1.0-rtsp \
# 	gstreamer1.0-tools \
# 	gstreamer1.0-vaapi \
# 	gstreamer1.0-x \
# 	libgstreamer-gl1.0-0 \
# 	libgstreamer-plugins-bad1.0-dev \
# 	libgstreamer-plugins-base1.0-dev \
# 	libgstreamer-plugins-good1.0-dev \
# 	libgstreamer1.0-dev

# QT_VERSION="${QT_VERSION:-6.6.3}"
# QT_PATH="${QT_PATH:-/opt/Qt}"
# QT_HOST="${QT_HOST:-linux}"
# QT_TARGET="${QT_TARGET:-desktop}"
# QT_ARCH="${QT_ARCH:-gcc_64}"
# QT_MODULES="${QT_MODULES:-qtcharts qtlocation qtpositioning qtspeech qt5compat qtmultimedia qtserialport qtimageformats qtshadertools qtconnectivity qtquick3d}"

# echo "QT_VERSION $QT_VERSION"
# echo "QT_PATH $QT_PATH"
# echo "QT_HOST $QT_HOST"
# echo "QT_TARGET $QT_TARGET"
# echo "QT_ARCH $QT_ARCH"
# echo "QT_MODULES $QT_MODULES"

# pip3 install setuptools wheel py7zr ninja cmake aqtinstall
# aqt install-qt ${QT_HOST} ${QT_TARGET} ${QT_VERSION} ${QT_ARCH} -O ${QT_PATH} -m ${QT_MODULES}
# export PATH=$(readlink -e ${QT_PATH}/${QT_VERSION}/${QT_ARCH}/bin/):$PATH
# export PKG_CONFIG_PATH=$(readlink -e ${QT_PATH}/${QT_VERSION}/${QT_ARCH}/lib/pkgconfig):$PKG_CONFIG_PATH
# export LD_LIBRARY_PATH=$(readlink -e ${QT_PATH}/${QT_VERSION}/${QT_ARCH}/lib):$LD_LIBRARY_PATH
# export QT_ROOT_DIR=$(readlink -e ${QT_PATH}/${QT_VERSION}/${QT_ARCH})
# export QT_PLUGIN_PATH=$(readlink -e ${QT_PATH}/${QT_VERSION}/${QT_ARCH}/plugins)
# export QML2_IMPORT_PATH=$(readlink -e ${QT_PATH}/${QT_VERSION}/${QT_ARCH}/qml)

# echo "PATH $PATH"
# echo "PKG_CONFIG_PATH $PKG_CONFIG_PATH"
# echo "LD_LIBRARY_PATH $LD_LIBRARY_PATH"
# echo "QT_ROOT_DIR $QT_ROOT_DIR"
# echo "QT_PLUGIN_PATH $QT_PLUGIN_PATH"
# echo "QML2_IMPORT_PATH $QML2_IMPORT_PATH"

# cmake -S /project/source -B . -G Ninja -DCMAKE_BUILD_TYPE=Debug -DQGC_BUILD_TESTING=ON -DQGC_STABLE_BUILD=OFF
# cmake --build . --target all --config Debug
# cmake --install . --config Debug
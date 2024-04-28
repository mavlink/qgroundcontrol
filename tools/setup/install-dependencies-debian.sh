#! /usr/bin/env bash

set -e

apt update -y --quiet

#Build Tools
DEBIAN_FRONTEND=noninteractive apt -y --quiet install \
	appstream \
	binutils \
	build-essential \
	ccache \
	cmake \
	cppcheck \
	file \
	g++ \
	gcc \
	gdb \
	git \
	libfuse2 \
	libtool \
	locales \
	make \
	ninja-build \
	patchelf \
	pkgconf \
	python3 \
	python3-pip \
	rsync

#Qt Required
DEBIAN_FRONTEND=noninteractive apt -y --quiet install \
	libdrm-dev \
	libgl1-mesa-dev \
	libpulse-dev \
	libxcb-cursor0 \
	libxcb-glx0 \
	libxcb-icccm4 \
	libxcb-image0 \
	libxcb-keysyms1 \
	libxcb-randr0 \
	libxcb-render-util0 \
	libxcb-render0 \
	libxcb-shape0 \
	libxcb-shm0 \
	libxcb-sync1 \
	libxcb-util1 \
	libxcb-xfixes0 \
	libxcb-xinerama0 \
	libxcb-xkb-dev \
	libxcb1 \
	libxkbcommon-dev \
	libxkbcommon-x11-0 \
	libzstd-dev

#QGC
DEBIAN_FRONTEND=noninteractive apt -y --quiet install \
	flite \
	libsdl2-dev \
	libspeechd2 \
	speech-dispatcher \
	speech-dispatcher-flite

#GStreamer
DEBIAN_FRONTEND=noninteractive apt -y --quiet install \
	gstreamer1.0-alsa \
	gstreamer1.0-gl \
	gstreamer1.0-gtk3 \
	gstreamer1.0-libav \
	gstreamer1.0-plugins-bad \
	gstreamer1.0-plugins-base \
	gstreamer1.0-plugins-good \
	gstreamer1.0-plugins-rtp \
	gstreamer1.0-plugins-ugly \
	gstreamer1.0-rtsp \
	gstreamer1.0-tools \
	gstreamer1.0-vaapi \
	gstreamer1.0-x \
	libgstreamer-gl1.0-0 \
	libgstreamer-plugins-bad1.0-dev \
	libgstreamer-plugins-base1.0-dev \
	libgstreamer-plugins-good1.0-dev \
	libgstreamer1.0-dev

#Vulkan
wget -qO - http://packages.lunarg.com/lunarg-signing-key-pub.asc | apt-key add -
wget -qO /etc/apt/sources.list.d/lunarg-vulkan-focal.list http://packages.lunarg.com/vulkan/lunarg-vulkan-focal.list
apt update
apt install vulkan-sdk

#Compiler
add-apt-repository ppa:ubuntu-toolchain-r/ppa
DEBIAN_FRONTEND=noninteractive apt -y --quiet install \
	gcc-13 \
	g++-13
update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100 --slave /usr/bin/g++ g++ /usr/bin/g++-13 --slave /usr/bin/gcov gcov /usr/bin/gcov-13
update-alternatives --set gcc /usr/bin/gcc-13

#Build Tools
python3 -m pip install --user ninja cmake

#Caching
wget --quiet https://github.com/ccache/ccache/releases/download/v4.9.1/ccache-4.9.1-linux-x86_64.tar.xz
tar -xvf ccache-*-linux-x86_64.tar.xz
cd ccache-*-linux-x86_64
make install

#Unit Tests
DEBIAN_FRONTEND=noninteractive apt -y --quiet install \
	xvfb \
	x11-xserver-utils

apt clean

#! /usr/bin/env bash

set -e

apt-get update -y --quiet

#Build Tools
DEBIAN_FRONTEND=noninteractive apt-get -y --quiet --no-install-recommends install \
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
	make \
	ninja-build \
	rsync \
    binutils \
    locales \
    patchelf

#Qt Required
DEBIAN_FRONTEND=noninteractive apt-get -y --quiet install \
	libxcb-xinerama0 \
    libxkbcommon-x11-0 \
    libxcb-cursor0

#QGC
DEBIAN_FRONTEND=noninteractive apt-get -y --quiet install \
	libsdl2-dev \
	libspeechd2 \
	flite \
	speech-dispatcher \
	speech-dispatcher-flite

#GStreamer
DEBIAN_FRONTEND=noninteractive apt-get -y --quiet install \
	libgstreamer1.0-dev \
	libgstreamer-plugins-base1.0-dev \
	libgstreamer-plugins-good1.0-dev \
	libgstreamer-plugins-bad1.0-dev \
	gstreamer1.0-plugins-base \
	gstreamer1.0-plugins-good \
	gstreamer1.0-plugins-bad \
	gstreamer1.0-plugins-ugly \
	gstreamer1.0-plugins-rtp \
	gstreamer1.0-libav \
	gstreamer1.0-tools \
	gstreamer1.0-x \
	gstreamer1.0-alsa \
	gstreamer1.0-gl \
	gstreamer1.0-gtk3 \
	gstreamer1.0-qt5 \
	gstreamer1.0-pulseaudio \
	gstreamer1.0-gl \
	gstreamer1.0-libav \
	gstreamer1.0-vaapi \
	gstreamer1.0-rtsp \
    libdrm-dev

#! /usr/bin/env bash

set -e

sudo apt-get update -y --quiet
sudo DEBIAN_FRONTEND=noninteractive apt-get -y --quiet --no-install-recommends install \
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
	libsdl2-dev \
    libgstreamer-plugins-base1.0-dev \
    libgstreamer1.0-0:amd64 \
    libgstreamer1.0-dev \
    binutils \
    patchelf \
    libxcb-xinerama0 \
    libxkbcommon-x11-0 \
    libxcb-cursor0 \
    libdrm-dev \
	libspeechd2 \
	flite \
	speech-dispatcher \
	speech-dispatcher-flite

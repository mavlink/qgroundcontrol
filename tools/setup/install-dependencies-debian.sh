#!/usr/bin/env bash

set -e

apt update -y --quiet

# Build Tools
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
    gnupg \
    gnupg2 \
    libfuse2 \
    libfuse3-3 \
    libtool \
    locales \
    make \
    ninja-build \
    patchelf \
    pkgconf \
    python3 \
    python3-pip \
    rsync

# Qt Required - https://doc.qt.io/qt-6/linux-requirements.html
DEBIAN_FRONTEND=noninteractive apt -y --quiet install \
    libfontconfig1 \
    libfreetype6 \
    libx11-6 \
    libx11-xcb1 \
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
    libxcb-xkb1 \
    libxcb1 \
    libxext6 \
    libxfixes3 \
    libxi6 \
    libxkbcommon-x11-0 \
    libxkbcommon0 \
    libxrender1

# GStreamer
DEBIAN_FRONTEND=noninteractive apt -y --quiet install \
    libgstreamer1.0-dev \
    libgstreamer-plugins-bad1.0-0 \
    libgstreamer-plugins-base1.0-dev \
    libgstreamer-plugins-good1.0-0 \
    libgstreamer-gl1.0-0 \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-ugly \
    gstreamer1.0-plugins-rtp \
    gstreamer1.0-gl \
    gstreamer1.0-libav \
    gstreamer1.0-rtsp \
    gstreamer1.0-vaapi \
    gstreamer1.0-x

# Additional
DEBIAN_FRONTEND=noninteractive apt -y --quiet install \
    flite1-dev \
    libasound2-dev \
    libass-dev \
    libdrm-dev \
    libegl1-mesa-dev \
    libgbm-dev \
    libgeographic-dev \
    libgl1-mesa-dev \
    libgles2-mesa-dev \
    libglfw3-dev \
    libopenal-dev \
    libpulse-dev \
    libsdl2-dev \
    libspeechd-dev \
    libunwind-dev \
    libva-dev \
    libvdpau-dev \
    libwayland-dev \
    libx11-dev \
    libzstd-dev \
    mesa-va-drivers \
    speech-dispatcher \
    speech-dispatcher-espeak \
    speech-dispatcher-espeak-ng \
    speech-dispatcher-flite \
    vainfo

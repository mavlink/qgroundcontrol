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
    libgstreamer-plugins-bad1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libgstreamer-plugins-good1.0-dev \
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

if apt-cache show gstreamer1.0-qt6 >/dev/null 2>&1 && apt-cache show gstreamer1.0-qt6 2>/dev/null | grep -q "^Package: gstreamer1.0-qt6"; then
    DEBIAN_FRONTEND=noninteractive apt-get install -y --quiet gstreamer1.0-qt6
fi

# Exiv2
DEBIAN_FRONTEND=noninteractive apt -y --quiet install \
    libbrotli-dev \
    libcurl4-openssl-dev \
    libexiv2-dev \
    libexpat1-dev \
    libinih-dev \
    libssh-dev \
    libxml2-utils \
    libz-dev \
    zlib1g-dev

# Additional
DEBIAN_FRONTEND=noninteractive apt -y --quiet install \
    flite1-dev \
    intel-media-va-driver \
    libasound2-dev \
    libass-dev \
    libdrm-dev \
    libegl1-mesa-dev \
    libgbm-dev \
    libgl1-mesa-dev \
    libgl-dev \
    libglx-dev \
    libgles2-mesa-dev \
    libglu1-mesa-dev \
    libglfw3-dev \
    libgraphene-1.0-dev \
    libopenal-dev \
    libpulse-dev \
    libsdl2-dev \
    libspeechd-dev \
    libshp-dev \
    libunwind-dev \
    libva-dev \
    libvdpau-dev \
    libvpx-dev \
    libwayland-dev \
    libx11-dev \
    libzstd-dev \
    mesa-common-dev \
    mesa-utils \
    mesa-va-drivers \
    mesa-vdpau-drivers \
    mesa-vulkan-drivers \
    speech-dispatcher \
    speech-dispatcher-espeak \
    speech-dispatcher-espeak-ng \
    speech-dispatcher-flite \
    vainfo

if apt-cache show libvpl-dev >/dev/null 2>&1 && apt-cache show libvpl-dev 2>/dev/null | grep -q "^Package: libvpl-dev"; then
    DEBIAN_FRONTEND=noninteractive apt-get install -y --quiet libvpl-dev
fi

# Geo
if apt-cache show libgeographic-dev >/dev/null 2>&1 && apt-cache show libgeographic-dev 2>/dev/null | grep -q "^Package: libgeographic-dev"; then
    DEBIAN_FRONTEND=noninteractive apt-get install -y --quiet libgeographic-dev
elif apt-cache show libgeographiclib-dev >/dev/null 2>&1 && apt-cache show libgeographiclib-dev 2>/dev/null | grep -q "^Package: libgeographiclib-dev"; then
    DEBIAN_FRONTEND=noninteractive apt-get install -y --quiet libgeographiclib-dev
fi

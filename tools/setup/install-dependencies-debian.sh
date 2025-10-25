#!/usr/bin/env bash
set -euo pipefail
export DEBIAN_FRONTEND=noninteractive

apt-get update -y -qq
apt-get install -y -qq --no-install-recommends \
    software-properties-common \
    gnupg2 \
    ca-certificates

# Enable the “universe” component (needed for several dev packages)
add-apt-repository -y universe
apt-get update -y -qq

# --------------------------------------------------------------------
# Core build tools
# --------------------------------------------------------------------
apt-get install -y -qq --no-install-recommends \
    appstream \
    binutils \
    build-essential \
    ccache \
    cmake \
    cppcheck \
    file \
    gdb \
    git \
    libfuse2 \
    fuse3 \
    libtool \
    locales \
    mold \
    ninja-build \
    patchelf \
    pipx \
    pkgconf \
    python3 \
    python3-pip \
    rsync \
    wget \
    zsync

pipx ensurepath
pipx install cmake
pipx install ninja

# --------------------------------------------------------------------
# Qt6 compile/runtime dependencies
# See: https://doc.qt.io/qt-6/linux-requirements.html
# --------------------------------------------------------------------
apt-get install -y -qq --no-install-recommends \
    libatspi2.0-dev \
    libfontconfig1-dev \
    libfreetype-dev \
    libgtk-3-dev \
    libsm-dev \
    libx11-dev \
    libx11-xcb-dev \
    libxcb-cursor-dev \
    libxcb-glx0-dev \
    libxcb-icccm4-dev \
    libxcb-image0-dev \
    libxcb-keysyms1-dev \
    libxcb-present-dev \
    libxcb-randr0-dev \
    libxcb-render-util0-dev \
    libxcb-render0-dev \
    libxcb-shape0-dev \
    libxcb-shm0-dev \
    libxcb-sync-dev \
    libxcb-util-dev \
    libxcb-xfixes0-dev \
    libxcb-xinerama0-dev \
    libxcb-xkb-dev \
    libxcb1-dev \
    libxext-dev \
    libxfixes-dev \
    libxi-dev \
    libxkbcommon-dev \
    libxkbcommon-x11-dev \
    libxrender-dev \
    libunwind-dev

# --------------------------------------------------------------------
# GStreamer (video/telemetry streaming)
# --------------------------------------------------------------------
apt-get install -y -qq --no-install-recommends \
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
    gstreamer1.0-x

# Optional – only present on Ubuntu 22.04+; skip gracefully otherwise
if apt-cache show gstreamer1.0-qt6 >/dev/null 2>&1; then
    apt-get install -y -qq --no-install-recommends gstreamer1.0-qt6
fi

# --------------------------------------------------------------------
# SDL
# --------------------------------------------------------------------
apt-get install -y -qq --no-install-recommends \
    libusb-1.0-0-dev

# --------------------------------------------------------------------
# Miscellaneous
# --------------------------------------------------------------------
apt-get install -y -qq --no-install-recommends \
    libvulkan-dev \
    libpipewire-0.3-dev

# --------------------------------------------------------------------
# Clean‑up
# --------------------------------------------------------------------
apt-get clean
rm -rf /var/lib/apt/lists/*

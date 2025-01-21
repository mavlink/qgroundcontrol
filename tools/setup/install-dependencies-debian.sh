#!/usr/bin/env bash

set -e

apt-get update -y --quiet

# Build Tools
DEBIAN_FRONTEND=noninteractive apt-get -y --quiet install \
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
    rsync \
    wget2 \
    zsync

# Qt Required - https://doc.qt.io/qt-6/linux-requirements.html
DEBIAN_FRONTEND=noninteractive apt-get -y --quiet install \
    libatspi2.0-dev \
    libfontconfig1-dev \
    libfreetype-dev \
    libglib2.0-dev \
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
    libxrender-dev

DEBIAN_FRONTEND=noninteractive apt-get -y --quiet install \
    libunwind-dev

# GStreamer
DEBIAN_FRONTEND=noninteractive apt-get -y --quiet install \
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
DEBIAN_FRONTEND=noninteractive apt-get -y --quiet install \
    libbrotli-dev \
    libcurl4-openssl-dev \
    libexiv2-dev \
    libexpat1-dev \
    libfmt-dev \
    libinih-dev \
    libssh-dev \
    libxml2-utils \
    libz-dev \
    zlib1g-dev

# Speech
DEBIAN_FRONTEND=noninteractive apt-get -y --quiet install \
    flite \
    flite1-dev \
    libflite1 \
    libspeechd-dev \
    speech-dispatcher \
    speech-dispatcher-audio-plugins \
    speech-dispatcher-espeak \
    speech-dispatcher-espeak-ng \
    speech-dispatcher-flite

# Joystick
DEBIAN_FRONTEND=noninteractive apt-get -y --quiet install \
    libsdl2-dev

# Shapelib
DEBIAN_FRONTEND=noninteractive apt-get -y --quiet install \
    libshp-dev

# DNS
# DEBIAN_FRONTEND=noninteractive apt-get -y --quiet install libavahi-compat-libdnssd-dev

# Additional
DEBIAN_FRONTEND=noninteractive apt-get -y --quiet install \
    bison \
    flex \
    gobject-introspection \
    gvfs \
    intel-media-va-driver \
    libasound2-dev \
    libass-dev \
    libdrm-dev \
    libcairo-dev \
    libelf-dev \
    libegl1-mesa-dev \
    libgbm-dev \
    libgl-dev \
    libgl1-mesa-dev \
    libgles-dev \
    libgles2-mesa-dev \
    libglew-dev \
    libglfw3-dev \
    libglu1-mesa-dev \
    libglvnd-dev \
    libglx-dev \
    libglx-mesa0 \
    libgudev-1.0-dev \
    libgraphene-1.0-dev \
    libharfbuzz-dev \
    libmp3lame-dev \
    libmjpegtools-dev \
    libjpeg-dev \
    libjson-glib-1.0-0 \
    libjson-glib-dev \
    libopenal-dev \
    libopenjp2-7-dev \
    libopus-dev \
    liborc-0.4-dev \
    libpng-dev \
    libpulse-dev \
    libsoup2.4-dev \
    libssl-dev \
    libtheora-dev \
    libva-dev \
    libvdpau-dev \
    libvorbis-dev \
    libvpx-dev \
    libwayland-dev \
    libwxgtk3.*-dev \
    libx264-dev \
    libx265-dev \
    libxcb-dri2-0-dev \
    libxcb-dri3-dev \
    libxcb-xf86dri0-dev \
    libxml2-dev \
    libzstd-dev \
    mesa-common-dev \
    mesa-utils \
    mesa-va-drivers \
    mesa-vdpau-drivers \
    mesa-vulkan-drivers \
    va-driver-all \
    vainfo \
    wayland-protocols

if apt-cache show libdav1d-dev >/dev/null 2>&1 && apt-cache show libdav1d-dev 2>/dev/null | grep -q "^Package: libdav1d-dev"; then
    DEBIAN_FRONTEND=noninteractive apt-get install -y --quiet libdav1d-dev
fi

if apt-cache show libvpl-dev >/dev/null 2>&1 && apt-cache show libvpl-dev 2>/dev/null | grep -q "^Package: libvpl-dev"; then
    DEBIAN_FRONTEND=noninteractive apt-get install -y --quiet libvpl-dev
fi

# Geo
if apt-cache show libgeographic-dev >/dev/null 2>&1 && apt-cache show libgeographic-dev 2>/dev/null | grep -q "^Package: libgeographic-dev"; then
    DEBIAN_FRONTEND=noninteractive apt-get install -y --quiet libgeographic-dev
elif apt-cache show libgeographiclib-dev >/dev/null 2>&1 && apt-cache show libgeographiclib-dev 2>/dev/null | grep -q "^Package: libgeographiclib-dev"; then
    DEBIAN_FRONTEND=noninteractive apt-get install -y --quiet libgeographiclib-dev
fi

# Vulkan
# Ubuntu 20.04
# wget -qO - http://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo apt-key add -
# wget -qO /etc/apt/sources.list.d/lunarg-vulkan-1.3.283-focal.list https://packages.lunarg.com/vulkan/1.3.283/lunarg-vulkan-1.3.283-focal.list

# Ubuntu 22.04
# wget -qO- https://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo tee /etc/apt/trusted.gpg.d/lunarg.asc
# wget -qO /etc/apt/sources.list.d/lunarg-vulkan-jammy.list http://packages.lunarg.com/vulkan/lunarg-vulkan-jammy.list

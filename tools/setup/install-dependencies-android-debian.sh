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
    rsync \
    openjdk-17-jdk \
    wget2 \
    unzip \
    android-sdk

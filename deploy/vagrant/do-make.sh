#!/bin/bash

set -e
set -x

# Must match the CONFIG used by do-configure.sh (Ninja is a single-config generator).
CONFIG="${CONFIG:-Release}"

SHADOW_BUILD_DIRNAME="shadow_build"
SHADOW_BUILD="$HOME/$SHADOW_BUILD_DIRNAME"

cd "$SHADOW_BUILD"

time cmake --build . --target all --config "$CONFIG"  # 75m
rm -rf AppDir
time cmake --install . --config "$CONFIG"

# make the build output visible on the host
rsync --delete -aPH "$SHADOW_BUILD"/ "/vagrant/$SHADOW_BUILD_DIRNAME"

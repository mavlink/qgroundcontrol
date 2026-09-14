#!/bin/bash

set -e
set -x

# Override with e.g. CONFIG=Debug ./do-configure.sh; do-make.sh honours the same variable.
CONFIG="${CONFIG:-Release}"

PYTHON="$HOME/venv-qgroundcontrol/bin/python3"

# Single source of truth for the Qt version is .github/build-config.json (also used by the Vagrantfile).
QT_VERSION=$("$PYTHON" -c 'import json, sys; print(json.load(open(sys.argv[1]))["qt"]["version"])' /vagrant/.github/build-config.json)
QT_ROOT="$HOME/Qt/$QT_VERSION/gcc_64"

# shadow_build in /home to avoid linking problems on vboxfs
SHADOW_BUILD="$HOME/shadow_build"
rm -rf "$SHADOW_BUILD"
mkdir "$SHADOW_BUILD"
cd "$SHADOW_BUILD"
time cmake \
  -S /vagrant \
  -B . \
  -G Ninja \
  -DCMAKE_BUILD_TYPE="$CONFIG" \
  -DQGC_BUILD_TESTING=OFF \
  -DQGC_STABLE_BUILD=OFF \
  -DPython3_EXECUTABLE="$PYTHON" \
  -DPython_EXECUTABLE="$PYTHON" \
  -DCMAKE_PREFIX_PATH="$QT_ROOT" # 34s

#!/usr/bin/env bash
set -euo pipefail

REPO="$HOME/qgroundcontrol"
BUILD="$REPO/build"
JOBS=8

cd "$REPO"

# If build dir missing or broken, reconfigure
if [ ! -f "$BUILD/build.ninja" ] || [ ! -d "$BUILD/CMakeFiles" ]; then
  echo "Configuring build directory..."
  rm -rf "$BUILD"
  mkdir -p "$BUILD"
  CC=/usr/bin/gcc CXX=/usr/bin/g++ CCACHE_DISABLE=1 \
    cmake -S "$REPO" -B "$BUILD" -GNinja \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DQt6_DIR=/home/mehmet/Qt/6.10.0/gcc_64/lib/cmake/Qt6 \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
fi

# Build only the QGroundControl target
ninja -C "$BUILD" -j${JOBS} QGroundControl

# Run the binary (try the usual locations)
if [ -x "$BUILD/QGroundControl" ]; then
  "$BUILD/QGroundControl"
elif [ -x "$BUILD/RelWithDebInfo/QGroundControl" ]; then
  "$BUILD/RelWithDebInfo/QGroundControl"
elif [ -x "$BUILD/AppDir/usr/bin/QGroundControl" ]; then
  "$BUILD/AppDir/usr/bin/QGroundControl"
else
  echo "QGroundControl binary not found; listing $BUILD"
  ls -la "$BUILD"
  exit 1
fi

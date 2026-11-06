#!/bin/bash
# Run a command with one Android ABI's Qt kit selected, for the Android CMake presets:
#   qgc-android arm64-v8a cmake --preset Android
# The image's default Qt stays the desktop one. Only the x86_64 image ships Android, because
# Google publishes the NDK for x86_64 Linux hosts only.
set -euo pipefail

if [[ "$(uname -m)" != x86_64 ]]; then
    echo "Android builds need the x86_64 qgc-dev image: the NDK runs only on x86_64 Linux." >&2
    echo "Run the container with --platform linux/amd64 (on Apple Silicon, with Rosetta)." >&2
    exit 1
fi
if [[ $# -lt 2 ]]; then
    echo "Usage: qgc-android <arm64-v8a|armeabi-v7a|x86_64> <command> [args...]" >&2
    exit 2
fi
case "$1" in
    arm64-v8a) kit=android_arm64_v8a ;;
    armeabi-v7a) kit=android_armv7 ;;
    x86_64) kit=android_x86_64 ;;
    *)
        echo "Unsupported Android ABI: $1 (arm64-v8a, armeabi-v7a or x86_64)" >&2
        exit 2
        ;;
esac
shift

# shellcheck source=/dev/null
source /etc/profile.d/qt.sh
# shellcheck source=/dev/null
source /etc/profile.d/qgc-android.sh
QT_TARGET_ROOT_DIR="$(dirname "${QT_ROOT_DIR}")/${kit}"
if [[ ! -f "${QT_TARGET_ROOT_DIR}/lib/cmake/Qt6/qt.toolchain.cmake" ]]; then
    echo "The ${kit} Qt kit is missing from ${QT_TARGET_ROOT_DIR}" >&2
    exit 1
fi
export QT_TARGET_ROOT_DIR
export QT_HOST_PATH="${QT_ROOT_DIR}"
export ANDROID_NDK="${ANDROID_NDK_ROOT}"
exec "$@"

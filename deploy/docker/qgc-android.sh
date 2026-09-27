#!/bin/bash
set -euo pipefail

if [[ "$(uname -m)" != x86_64 ]]; then
    echo "Android requires the official Linux x86_64 NDK: use qgc-dev with --platform linux/amd64." >&2
    echo "On Apple Silicon, enable Docker Desktop x86-64 emulation/Rosetta." >&2
    exit 1
fi
if [[ $# -lt 2 ]]; then
    echo "Usage: qgc-android <arm64-v8a|armeabi-v7a|x86_64> <command> [args...]" >&2
    exit 2
fi
case "$1" in
    arm64-v8a|armeabi-v7a|x86_64) ;;
    *) echo "Unsupported Android ABI: $1" >&2; exit 2 ;;
esac
if [[ ! -f "/opt/qt-android/$1/lib/cmake/Qt6/qt.toolchain.cmake" ]]; then
    echo "Android Qt kit is not installed: $1. Use the linux/amd64 qgc-dev image." >&2
    exit 1
fi
# Android selection is command-scoped; the image's default Qt remains desktop.
# shellcheck source=/dev/null
source /opt/qgc-android/env.sh
export QT_TARGET_ROOT_DIR="/opt/qt-android/$1"
export ANDROID_ABIS="$1"
shift
exec "$@"

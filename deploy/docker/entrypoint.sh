#!/bin/bash
set -euo pipefail

BUILD_TYPE="${1:-${BUILD_TYPE:-Release}}"
ANDROID_ABIS="${ANDROID_ABIS:-arm64-v8a}"  # Options: arm64-v8a, armeabi-v7a, or both with semicolon

case "${BUILD_TYPE}" in
    Release|Debug|RelWithDebInfo|MinSizeRel) ;;
    *)
        echo "Error: Invalid BUILD_TYPE: ${BUILD_TYPE}"
        echo "Usage: $0 [Release|Debug|RelWithDebInfo|MinSizeRel]"
        exit 1
        ;;
esac

if [[ -n "${ANDROID_SDK_ROOT:-}" ]]; then
    # Validate required Android environment variables
    for var in QT_HOST_PATH QT_ROOT_DIR_ARM64 ANDROID_SDK_ROOT; do
        if [[ -z "${!var:-}" ]]; then
            echo "Error: Required environment variable $var is not set" >&2
            exit 1
        fi
    done
    echo "Building QGroundControl for Android (${BUILD_TYPE})..."
    "${QT_ROOT_DIR_ARM64}/bin/qt-cmake" -S /project/source -B /project/build -G Ninja \
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
        -DQT_HOST_PATH="${QT_HOST_PATH}" \
        -DQT_ANDROID_ABIS="${ANDROID_ABIS}" \
        -DANDROID_SDK_ROOT="${ANDROID_SDK_ROOT}" \
        -DQGC_CUSTOM_GST_PACKAGE=OFF \
        -DQT_ANDROID_SIGN_APK=OFF
    cmake --build /project/build --target all --config "${BUILD_TYPE}" --parallel
else
    echo "Building QGroundControl (${BUILD_TYPE})..."
    qt-cmake -S /project/source -B /project/build -G Ninja \
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
    cmake --build /project/build --target all --parallel
    cmake --install /project/build --config "${BUILD_TYPE}"
fi

echo "Build complete!"

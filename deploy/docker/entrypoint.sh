#!/bin/bash
set -euo pipefail

BUILD_TYPE="${1:-${BUILD_TYPE:-Release}}"

case "${BUILD_TYPE}" in
    Release|Debug|RelWithDebInfo|MinSizeRel) ;;
    *)
        echo "Error: Invalid BUILD_TYPE: ${BUILD_TYPE}"
        echo "Usage: $0 [Release|Debug|RelWithDebInfo|MinSizeRel]"
        exit 1
        ;;
esac

if [[ -n "${ANDROID_SDK_ROOT:-}" ]]; then
    echo "Building QGroundControl for Android (${BUILD_TYPE})..."
    qt-cmake -S /project/source -B /project/build -G Ninja \
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
        -DQT_HOST_PATH="${QT_HOST_PATH}" \
        -DQT_ANDROID_ABIS="armeabi-v7a;arm64-v8a" \
        -DANDROID_BUILD_TOOLS="${ANDROID_BUILD_TOOLS_DIR}" \
        -DANDROID_SDK_ROOT="${ANDROID_SDK_ROOT}" \
        -DQT_ANDROID_SIGN_APK=OFF \
        -DCMAKE_TOOLCHAIN_FILE="${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake"
    cmake --build /project/build --target all --config "${BUILD_TYPE}" --parallel
else
    echo "Building QGroundControl (${BUILD_TYPE})..."
    qt-cmake -S /project/source -B /project/build -G Ninja \
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
    cmake --build /project/build --target all --parallel
    cmake --install /project/build --config "${BUILD_TYPE}"
fi

echo "Build complete!"

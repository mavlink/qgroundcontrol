#!/bin/bash
set -euo pipefail

# Determine build type
BUILD_TYPE="${1:-${BUILD_TYPE:-Release}}"
ANDROID_ABIS="${ANDROID_ABIS:-arm64-v8a}"  # Options: arm64-v8a, armeabi-v7a, or both with semicolon

# Validate build type
VALID_BUILD_TYPES=("Release" "Debug" "RelWithDebInfo" "MinSizeRel")
if [[ ! " ${VALID_BUILD_TYPES[*]} " =~ .*\ ${BUILD_TYPE}\ .* ]]; then
    echo "ERROR: Invalid BUILD_TYPE: ${BUILD_TYPE}"
    echo "USAGE: $0 ${VALID_BUILD_TYPES[*]}"
    exit 1
fi

# Build for Android if ANDROID_SDK_ROOT is set
if [[ -n "${ANDROID_SDK_ROOT:-}" ]]; then
    # Validate required Android environment variables
    for var in QT_HOST_PATH QT_ROOT_DIR_ARM64 ANDROID_SDK_ROOT; do
        if [[ -z "${!var:-}" ]]; then
            echo "Error: Required environment variable $var is not set" >&2
            exit 1
        fi
    done
    echo "Building QGroundControl for Android (${BUILD_TYPE})..."

    # Configure and build for Android
    "${QT_ROOT_DIR_ARM64}/bin/qt-cmake" -S /project/source -B /project/build -G Ninja \
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DANDROID_SDK_ROOT="${ANDROID_SDK_ROOT}" \
        -DANDROID_PLATFORM=android-${ANDROID_PLATFORM} \
        -DQT_HOST_PATH="${QT_HOST_PATH}" \
        -DQT_ANDROID_ABIS="${ANDROID_ABIS}" \
        -DQT_ANDROID_SIGN_APK=OFF
    cmake --build /project/build --target all --config "${BUILD_TYPE}" --parallel "$(nproc)"
else
# Build for desktop
    echo "Building QGroundControl (${BUILD_TYPE})..."
    # Configure, build, and install
    qt-cmake -S /project/source -B /project/build -G Ninja \
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    cmake --build /project/build --target all --parallel "$(nproc)"
    cmake --install /project/build --config "${BUILD_TYPE}" --strip
fi

echo "Build complete!"

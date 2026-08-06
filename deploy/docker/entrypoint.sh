#!/bin/bash
# Unified build entrypoint for the QGC Docker images (deploy/docker/Dockerfile).
# The target is auto-detected from environment baked into each image stage:
#   ANDROID_SDK_ROOT set -> Android APK        (target: android)
#   SYSROOT set          -> aarch64 cross-build (target: linux-cross)
#   neither              -> native x86_64 build (target: linux)
set -euo pipefail

# linuxdeploy/appimagetool are AppImages that FUSE-mount themselves; /dev/fuse
# isn't usable in the RunsOn container, so make them extract-and-run instead.
export APPIMAGE_EXTRACT_AND_RUN=1

. /usr/local/lib/qgc/build-type.sh

BUILD_TYPE="${1:-${BUILD_TYPE:-Release}}"
validate_build_type "${BUILD_TYPE}" || exit 1

# Parallelism: honor JOBS (e.g. JOBS=4 to cap on a shared host); default to all
# cores via --parallel. CI runs on dedicated instances where full throughput is
# desired; set JOBS to throttle.
if [[ -n "${JOBS:-}" ]]; then
    BUILD_PARALLEL=(-j "${JOBS}")
else
    BUILD_PARALLEL=(--parallel)
fi

require_env() {
    for var in "$@"; do
        if [[ -z "${!var:-}" ]]; then
            echo "Error: Required environment variable ${var} is not set" >&2
            exit 1
        fi
    done
}

if [[ -n "${ANDROID_SDK_ROOT:-}" ]]; then
    ANDROID_ABIS="${ANDROID_ABIS:-arm64-v8a}"  # arm64-v8a, armeabi-v7a, or both (semicolon-separated)
    require_env QT_HOST_PATH QT_ROOT_DIR_ARM64 ANDROID_SDK_ROOT
    echo "Building QGroundControl for Android (${BUILD_TYPE})..."
    "${QT_ROOT_DIR_ARM64}/bin/qt-cmake" -S /project/source -B /project/build -G Ninja \
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
        -DPython3_EXECUTABLE=/opt/qgc-venv/bin/python \
        -DQT_HOST_PATH="${QT_HOST_PATH}" \
        -DQT_ANDROID_ABIS="${ANDROID_ABIS}" \
        -DANDROID_SDK_ROOT="${ANDROID_SDK_ROOT}" \
        -DQT_ANDROID_SIGN_APK=OFF
    cmake --build /project/build --target all --config "${BUILD_TYPE}" "${BUILD_PARALLEL[@]}"
elif [[ -n "${SYSROOT:-}" ]]; then
    require_env QT_HOST_PATH QT_ROOT_DIR SYSROOT
    # qt-cross.sh exports QT_HOST_PATH/QT_ROOT_DIR; an empty QT_HOST_PATH silently
    # breaks AUTOMOC with confusing "moc: bad input file" errors mid-build.
    TOOLCHAIN=/project/source/cmake/platform/Linux-aarch64-toolchain.cmake
    if [[ ! -f "${TOOLCHAIN}" ]]; then
        echo "Error: toolchain file not found at ${TOOLCHAIN}" >&2
        echo "Mount the QGC source tree at /project/source." >&2
        exit 1
    fi
    echo "Cross-building QGroundControl (${BUILD_TYPE}) aarch64 on $(uname -m)"
    echo "  QT_HOST_PATH: ${QT_HOST_PATH}"
    echo "  QT_ROOT_DIR:  ${QT_ROOT_DIR}   (target)"
    echo "  SYSROOT:      ${SYSROOT}"
    if [[ "${CLEAN_BUILD:-0}" == "1" ]]; then
        echo "==> CLEAN_BUILD=1: removing /project/build/*"
        rm -rf /project/build/*
    fi
    # qt-cmake (host) loads target Qt's cmake config and feeds it the toolchain.
    "${QT_HOST_PATH}/bin/qt-cmake" \
        -S /project/source -B /project/build -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN}" \
        -DCMAKE_PREFIX_PATH="${QT_ROOT_DIR}" \
        -DQT_HOST_PATH="${QT_HOST_PATH}" \
        -DQGC_AARCH64_SYSROOT="${SYSROOT}" \
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
        -DQGC_CREATE_APPIMAGE=OFF \
        -DPython3_EXECUTABLE=/opt/qgc-venv/bin/python
    cmake --build /project/build --target all "${BUILD_PARALLEL[@]}"
    cmake --install /project/build --config "${BUILD_TYPE}"
else
    echo "Building QGroundControl (${BUILD_TYPE})..."
    # CMake derives the native package method (DEB/RPM via CPack, Arch via
    # makepkg) and the appimagelint default from the detected distro
    # (cmake/modules/LinuxDistro.cmake), so no per-distro flags are passed here.
    qt-cmake -S /project/source -B /project/build -G Ninja \
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
        -DPython3_EXECUTABLE=/opt/qgc-venv/bin/python
    cmake --build /project/build --target all "${BUILD_PARALLEL[@]}"
    cmake --install /project/build --config "${BUILD_TYPE}"

    # One packaging entry point; the distro→method dispatch lives in
    # cmake/install/Install.cmake (qgc-package target).
    echo "Creating native package..."
    cmake --build /project/build --target qgc-package
fi

echo "Build complete!"

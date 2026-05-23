#!/bin/bash
# Cross-build QGroundControl for Linux aarch64 inside the Docker image built
# from deploy/docker/Dockerfile-build-ubuntu-aarch64.
#
# Mount the source tree at /project/source and the build dir at /project/build.
set -euo pipefail

BUILD_TYPE="${1:-${BUILD_TYPE:-Release}}"

case "${BUILD_TYPE}" in
    Release|Debug|RelWithDebInfo|MinSizeRel) ;;
    *)
        echo "Error: Invalid BUILD_TYPE: ${BUILD_TYPE}" >&2
        echo "Usage: $0 [Release|Debug|RelWithDebInfo|MinSizeRel]" >&2
        exit 1
        ;;
esac

# /etc/profile.d/qt-cross.sh exports QT_HOST_PATH and QT_ROOT_DIR; verify the
# pieces actually landed before invoking cmake (an empty QT_HOST_PATH silently
# breaks AUTOMOC with confusing "moc: bad input file" errors mid-build).
for var in QT_HOST_PATH QT_ROOT_DIR SYSROOT; do
    if [[ -z "${!var:-}" ]]; then
        echo "Error: Required environment variable ${var} is not set" >&2
        exit 1
    fi
done

TOOLCHAIN=/project/source/cmake/platform/Linux-aarch64-toolchain.cmake
if [[ ! -f "${TOOLCHAIN}" ]]; then
    echo "Error: toolchain file not found at ${TOOLCHAIN}" >&2
    echo "Mount the QGC source tree at /project/source." >&2
    exit 1
fi

echo "Cross-building QGroundControl (${BUILD_TYPE}) aarch64 on $(uname -m)"
echo "  cmake:        $(cmake --version | head -1)"
echo "  QT_HOST_PATH: ${QT_HOST_PATH}"
echo "  QT_ROOT_DIR:  ${QT_ROOT_DIR}   (target)"
echo "  SYSROOT:      ${SYSROOT}"
echo "  toolchain:    ${TOOLCHAIN}"

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
    -DPython3_EXECUTABLE=/opt/qgc-venv/bin/python

JOBS="${JOBS:-$(( $(nproc) > 1 ? $(nproc) / 2 : 1 ))}"
echo "  jobs:         ${JOBS}"

cmake --build /project/build --target all -j "${JOBS}"
cmake --install /project/build --config "${BUILD_TYPE}"

echo "Cross-build complete: /project/build (target: aarch64)"

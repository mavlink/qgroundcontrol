#!/usr/bin/env bash
# Build + run a QGroundControl Docker builder image for a given variant.
#
# Usage:
#   deploy/docker/run-docker.sh <variant> [build-type]
#     variants:   ubuntu | ubuntu-2204 | aarch64 | android
#     build-type: Release (default) | Debug | RelWithDebInfo | MinSizeRel
#
# Env:
#   IMAGE_NAME   Override the image tag (default: per-variant below)
#   BUILD_DIR    Host build output dir (aarch64 default: <repo>/build-aarch64)
#   CLEAN_BUILD  Set 1 to wipe the build dir before configuring (aarch64 only)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
DOCKERFILE="${SCRIPT_DIR}/Dockerfile"

usage() {
    echo "Usage: $0 <ubuntu|ubuntu-2204|aarch64|android> [build-type]" >&2
    exit 1
}

VARIANT="${1:-}"
BUILD_TYPE="${2:-Release}"
[[ -z "${VARIANT}" ]] && usage

run_flags=()
build_args=()
case "${VARIANT}" in
    ubuntu)
        target="linux";       default_image="qgc-ubuntu-docker";         run_flags=(--fuse) ;;
    ubuntu-2204)
        # 22.04 reuses the `linux` target, overriding the base + toolchain args.
        target="linux";       default_image="qgc-ubuntu-2204-docker";    run_flags=(--fuse)
        build_args=(
            --build-arg "UBUNTU_REF=ubuntu:22.04@sha256:4f838adc7181d9039ac795a7d0aba05a9bd9ecd480d294483169c5def983b64d"
            --build-arg "APT_EXTRA=gcc-12 g++-12"
            --build-arg "PIP_CMAKE=cmake>=3.25,<4"
            --build-arg "CC_PIN=gcc-12"
            --build-arg "CXX_PIN=g++-12"
        ) ;;
    aarch64)
        target="linux-cross"; default_image="qgc-ubuntu-aarch64-docker" ;;
    android)
        target="android";     default_image="qgc-android-docker" ;;
    *)
        echo "Unknown variant: ${VARIANT}" >&2; usage ;;
esac

IMAGE_NAME="${IMAGE_NAME:-${default_image}}"

docker build \
    --file "${DOCKERFILE}" \
    --target "${target}" \
    ${build_args[@]+"${build_args[@]}"} \
    -t "${IMAGE_NAME}" \
    "${SOURCE_DIR}"

run_env=(SOURCE_DIR="${SOURCE_DIR}")
if [[ "${VARIANT}" == "aarch64" ]]; then
    BUILD_DIR="${BUILD_DIR:-${SOURCE_DIR}/build-aarch64}"
    # Throttle the local cross-build to half the cores by default (CI sets no JOBS
    # and so runs --parallel); override with JOBS=N.
    run_env+=(BUILD_DIR="${BUILD_DIR}" CLEAN_BUILD="${CLEAN_BUILD:-0}" \
        JOBS="${JOBS:-$(( $(nproc) > 1 ? $(nproc) / 2 : 1 ))}")
fi

env "${run_env[@]}" \
    "${SCRIPT_DIR}/_docker-exec.sh" ${run_flags[@]+"${run_flags[@]}"} "${IMAGE_NAME}" "${BUILD_TYPE}"

if [[ "${VARIANT}" == "aarch64" ]]; then
    echo "Output: ${BUILD_DIR}"
fi

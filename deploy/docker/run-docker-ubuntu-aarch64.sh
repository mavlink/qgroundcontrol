#!/usr/bin/env bash
# Build + run the aarch64 cross-compile Docker image, mounting the QGC source
# at /project/source and producing arm64 binaries in ./build-aarch64/.
#
# Usage:
#   deploy/docker/run-docker-ubuntu-aarch64.sh                 # Release
#   deploy/docker/run-docker-ubuntu-aarch64.sh Debug
#
# Env:
#   IMAGE_NAME  Docker image tag (default: qgc-ubuntu-aarch64-docker)
#   BUILD_DIR   Host-side build dir (default: <repo>/build-aarch64)
#   CLEAN_BUILD=1  Wipe BUILD_DIR before configuring.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
IMAGE_NAME="${IMAGE_NAME:-qgc-ubuntu-aarch64-docker}"
BUILD_TYPE="${1:-Release}"
BUILD_DIR="${BUILD_DIR:-${SOURCE_DIR}/build-aarch64}"

mkdir -p "${BUILD_DIR}"

docker build \
    --file "${SCRIPT_DIR}/Dockerfile-build-ubuntu-aarch64" \
    -t "${IMAGE_NAME}" \
    "${SOURCE_DIR}"

# --rm so a failed build leaves no zombie container; --user matches host UID
# so the on-disk build artifacts aren't owned by root.
docker run --rm \
    --user "$(id -u):$(id -g)" \
    -e CLEAN_BUILD="${CLEAN_BUILD:-0}" \
    -v "${SOURCE_DIR}:/project/source:ro" \
    -v "${BUILD_DIR}:/project/build" \
    "${IMAGE_NAME}" "${BUILD_TYPE}"

echo "Output: ${BUILD_DIR}"

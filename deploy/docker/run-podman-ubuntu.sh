#!/usr/bin/env bash

# Exit immediately if a command exits with a non-zero status
set -euo pipefail

# Define variables for better maintainability
PARALLEL_BUILD_AMOUNT=$(nproc --all)
DOCKERFILE_PATH="./deploy/docker/Containerfile-build-ubuntu"
IMAGE_NAME="qgc-ubuntu-docker"
SOURCE_DIR=$(dirname "$(readlink -f "$0")" | rev | cut -f3- -d'/' | rev)
BUILD_DIR="$SOURCE_DIR/build"

# Create the build directory if it doesn't exist
if [[ ! -d "$BUILD_DIR" ]]; then
    mkdir -p "$BUILD_DIR"
fi

# Build the Docker image
podman build \
    --jobs="${PARALLEL_BUILD_AMOUNT}" \
    --file "${DOCKERFILE_PATH}" \
    --tag "${IMAGE_NAME}" \
    "${SOURCE_DIR}"

# Run the Docker container with necessary permissions and volume mounts
podman run \
    --rm \
    --cap-add SYS_ADMIN \
    --device /dev/fuse \
    --security-opt apparmor:unconfined \
    -v "${SOURCE_DIR}:/project/source" \
    -v "${BUILD_DIR}:/project/build" \
    "${IMAGE_NAME}"

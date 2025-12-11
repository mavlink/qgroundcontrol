#!/usr/bin/env bash

# Exit immediately if a command exits with a non-zero status
set -euo pipefail

# Determine the build directory
if [[ -z "${BUILD_DIR:-}" ]]; then
    BUILD_DIR="$(dirname "$(readlink -f "$0")" | rev | cut -f3- -d'/' | rev)/build"
fi

# Create the build directory if it doesn't exist
if [[ ! -d "$BUILD_DIR" ]]; then
    mkdir -p "$BUILD_DIR"
fi

# Define variables for better maintainability
PARALLEL_BUILD_AMOUNT=$(nproc --all)
DOCKERFILE_PATH="./deploy/docker/Dockerfile-build-android"
IMAGE_NAME="qgc-android-docker"
SOURCE_DIR=$(pwd)
BUILD_DIR="${SOURCE_DIR}/build"

# Build the Docker image for Android
podman build \
    --jobs="${PARALLEL_BUILD_AMOUNT}" \
    --file "${DOCKERFILE_PATH}" \
    --tag "${IMAGE_NAME}" \
    "${SOURCE_DIR}"

# Run the Docker container with adjusted mount points
podman run \
    --rm \
    -v "${SOURCE_DIR}:/project/source" \
    -v "${BUILD_DIR}:/project/build" \
    "${IMAGE_NAME}"

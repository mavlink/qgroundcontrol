#!/usr/bin/env bash

# Exit immediately if a command exits with a non-zero status
set -euo pipefail

# Define variables for better maintainability
SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
SOURCE_DIR=$(echo "$SCRIPT_DIR" | rev | cut -f3- -d'/' | rev)
BUILD_DIR="$SOURCE_DIR/build"
PARALLEL_CPUS=$(nproc --all)
IMAGE_NAME="qgc-ubuntu-docker"
BUILD_TYPE="${1:-Release}"

# Build the Docker image for Linux
podman build \
    --file "${SCRIPT_DIR}/Containerfile-build-ubuntu" \
    --jobs "${PARALLEL_CPUS}" \
    --tag "${IMAGE_NAME}" \
    "${SOURCE_DIR}"

# Run the Docker container with necessary permissions and volume mounts
SOURCE_DIR="${SOURCE_DIR}" "${SCRIPT_DIR}/docker-run.sh" --fuse "${IMAGE_NAME}" "${BUILD_TYPE}"

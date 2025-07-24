#!/usr/bin/env bash

# Exit immediately if a command exits with a non-zero status
set -e

# Define variables for better maintainability
DOCKERFILE_PATH="./deploy/docker/Dockerfile-build-ubuntu"
IMAGE_NAME="qgc-ubuntu-docker"
SOURCE_DIR="$(pwd)"
BUILD_DIR="${SOURCE_DIR}/build"

# Build the Docker image
docker build --file "${DOCKERFILE_PATH}" -t "${IMAGE_NAME}" "${SOURCE_DIR}"

# Run the Docker container with necessary permissions and volume mounts
docker run \
  --rm \
  --cap-add SYS_ADMIN \
  --device /dev/fuse \
  --security-opt apparmor:unconfined \
  -v "${SOURCE_DIR}:/project/source" \
  -v "${BUILD_DIR}:/project/build" \
  "${IMAGE_NAME}"

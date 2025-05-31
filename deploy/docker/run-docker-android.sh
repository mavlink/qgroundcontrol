#!/usr/bin/env bash

# Exit immediately if a command exits with a non-zero status
set -e

# Define variables for better maintainability
DOCKERFILE_PATH="./deploy/docker/Dockerfile-build-android"
IMAGE_NAME="qgc-android-docker"
SOURCE_DIR="$(pwd)"
BUILD_DIR="${SOURCE_DIR}/build"

# Build the Docker image for Android
docker build --file "${DOCKERFILE_PATH}" -t "${IMAGE_NAME}" "${SOURCE_DIR}"

# Run the Docker container with adjusted mount points
docker run \
  --rm \
  -v "${SOURCE_DIR}:/project/source" \
  -v "${BUILD_DIR}:/project/build" \
  "${IMAGE_NAME}"

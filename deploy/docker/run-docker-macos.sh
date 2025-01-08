#!/usr/bin/env bash

# Exit immediately if a command exits with a non-zero status
set -e

# Build the macOS Docker-OSX image
docker build --file ./deploy/docker/Dockerfile-build-macos -t qgc-macos-docker .

# Run the Docker-OSX container with necessary volume mounts
docker run --rm \
  --cap-add SYS_ADMIN \
  --device /dev/fuse \
  --security-opt apparmor:unconfined \
  -v "${PWD}:/project/source" \
  -v "${PWD}/build:/project/build" \
  qgc-macos-docker

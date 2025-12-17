#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
IMAGE_NAME="qgc-ubuntu-docker"
BUILD_TYPE="${1:-Release}"

docker build \
  --file "${SCRIPT_DIR}/Dockerfile-build-ubuntu" \
  -t "${IMAGE_NAME}" \
  "${SOURCE_DIR}"

SOURCE_DIR="${SOURCE_DIR}" "${SCRIPT_DIR}/docker-run.sh" --fuse "${IMAGE_NAME}" "${BUILD_TYPE}"

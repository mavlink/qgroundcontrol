#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 [--fuse] <image-name> [build-type]"
    echo "  --fuse    Enable FUSE support (required for AppImage builds)"
    exit 1
}

FUSE_FLAGS=()
while [[ $# -gt 0 && "$1" == --* ]]; do
    case "$1" in
        --fuse)
            FUSE_FLAGS=(--cap-add SYS_ADMIN --device /dev/fuse --security-opt apparmor:unconfined)
            shift
            ;;
        --help|-h)
            usage
            ;;
        *)
            echo "Unknown option: $1"
            usage
            ;;
    esac
done

IMAGE_NAME="${1:-}"
BUILD_TYPE="${2:-Release}"
SOURCE_DIR="${SOURCE_DIR:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-${SOURCE_DIR}/build}"

[[ -z "${IMAGE_NAME}" ]] && usage

mkdir -p "${BUILD_DIR}"

docker run \
    --rm \
    "${FUSE_FLAGS[@]}" \
    -v "${SOURCE_DIR}:/project/source" \
    -v "${BUILD_DIR}:/project/build" \
    "${IMAGE_NAME}" \
    "${BUILD_TYPE}"

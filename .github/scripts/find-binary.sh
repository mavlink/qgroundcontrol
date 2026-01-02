#!/usr/bin/env bash
#
# Find QGroundControl binary in build directory
#
# Handles different build layouts:
#   - build/QGroundControl (single-config generators like Ninja)
#   - build/Release/QGroundControl (multi-config generators)
#   - build/Debug/QGroundControl (multi-config generators)
#   - Windows: .exe extension
#   - macOS: .app bundle
#
# Usage:
#   find-binary.sh [--build-dir DIR] [--build-type TYPE] [--platform PLATFORM]
#
# Outputs (for GitHub Actions):
#   binary_path - Full path to the binary
#   binary_name - Just the filename
#   binary_dir  - Directory containing the binary
#

set -euo pipefail

# Defaults
BUILD_DIR="build"
BUILD_TYPE=""
PLATFORM=""

usage() {
    cat >&2 <<EOF
Usage: $(basename "$0") [OPTIONS]

Options:
  --build-dir DIR       Build directory (default: build)
  --build-type TYPE     Build type (Release, Debug) - helps prioritize search
  --platform PLATFORM   Platform (linux, macos, windows, android, ios)
  -h, --help            Show this help

Outputs (GITHUB_OUTPUT):
  binary_path  - Full path to binary
  binary_name  - Filename only
  binary_dir   - Directory containing binary
EOF
    exit 1
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-dir) BUILD_DIR="$2"; shift 2 ;;
        --build-type) BUILD_TYPE="$2"; shift 2 ;;
        --platform) PLATFORM="$2"; shift 2 ;;
        -h|--help) usage ;;
        *) echo "Unknown option: $1" >&2; usage ;;
    esac
done

# Auto-detect platform
if [[ -z "$PLATFORM" ]]; then
    case "$(uname -s)" in
        Linux*)  PLATFORM="linux" ;;
        Darwin*) PLATFORM="macos" ;;
        MINGW*|MSYS*|CYGWIN*) PLATFORM="windows" ;;
        *) PLATFORM="linux" ;;
    esac
fi

# Determine binary name based on platform
case "$PLATFORM" in
    windows)
        BINARY_NAME="QGroundControl.exe"
        ;;
    macos)
        BINARY_NAME="QGroundControl.app"
        ;;
    *)
        BINARY_NAME="QGroundControl"
        ;;
esac

echo "Searching for $BINARY_NAME in $BUILD_DIR..."

# Build search order based on build type
SEARCH_PATHS=()

if [[ -n "$BUILD_TYPE" ]]; then
    # Prioritize the specified build type
    SEARCH_PATHS+=("$BUILD_DIR/$BUILD_TYPE/$BINARY_NAME")
    SEARCH_PATHS+=("$BUILD_DIR/$BINARY_NAME")
else
    # Default search order: single-config first, then Release, then Debug
    SEARCH_PATHS+=("$BUILD_DIR/$BINARY_NAME")
    SEARCH_PATHS+=("$BUILD_DIR/Release/$BINARY_NAME")
    SEARCH_PATHS+=("$BUILD_DIR/Debug/$BINARY_NAME")
    SEARCH_PATHS+=("$BUILD_DIR/RelWithDebInfo/$BINARY_NAME")
    SEARCH_PATHS+=("$BUILD_DIR/MinSizeRel/$BINARY_NAME")
fi

# Search for binary
FOUND_PATH=""
for path in "${SEARCH_PATHS[@]}"; do
    if [[ -e "$path" ]]; then
        FOUND_PATH="$path"
        break
    fi
done

# Fallback: use find command
if [[ -z "$FOUND_PATH" ]]; then
    echo "Standard paths not found, searching recursively..."
    FOUND_PATH=$(find "$BUILD_DIR" -name "$BINARY_NAME" -type f 2>/dev/null | head -1) || true

    # For macOS .app bundles, search for directories
    if [[ -z "$FOUND_PATH" && "$PLATFORM" == "macos" ]]; then
        FOUND_PATH=$(find "$BUILD_DIR" -name "$BINARY_NAME" -type d 2>/dev/null | head -1) || true
    fi
fi

if [[ -z "$FOUND_PATH" ]]; then
    echo "::error::Binary not found: $BINARY_NAME in $BUILD_DIR"
    echo "Searched paths:"
    for path in "${SEARCH_PATHS[@]}"; do
        echo "  - $path"
    done
    exit 1
fi

# Get absolute path
FOUND_PATH=$(realpath "$FOUND_PATH")
BINARY_DIR=$(dirname "$FOUND_PATH")

echo "Found: $FOUND_PATH"

# Output for GitHub Actions
if [[ -n "${GITHUB_OUTPUT:-}" ]]; then
    {
        echo "binary_path=$FOUND_PATH"
        echo "binary_name=$BINARY_NAME"
        echo "binary_dir=$BINARY_DIR"
    } >> "$GITHUB_OUTPUT"
fi

# Also output to stdout for non-CI usage
echo "binary_path=$FOUND_PATH"

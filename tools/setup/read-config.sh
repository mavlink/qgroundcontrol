#!/usr/bin/env bash
#
# Read build configuration from .github/build-config.json
#
# This script exports version variables from the centralized config file.
# Source this script to get the variables, or call with --get <key> to retrieve a single value.
#
# Usage:
#   source read-config.sh           # Export all variables
#   ./read-config.sh --get qt_version  # Get single value
#
# Exports:
#   QT_VERSION, QT_MODULES, GST_VERSION, XCODE_VERSION,
#   NDK_VERSION, JAVA_VERSION, ANDROID_PLATFORM, CCACHE_VERSION

set -euo pipefail

# Find the repo root (location of .github/build-config.json)
find_repo_root() {
    local dir="$1"
    while [[ "$dir" != "/" ]]; do
        if [[ -f "$dir/.github/build-config.json" ]]; then
            echo "$dir"
            return 0
        fi
        dir=$(dirname "$dir")
    done
    return 1
}

# Get a value from the JSON config using Python (more portable than jq)
get_config_value() {
    local key="$1"
    local config_file="$2"
    python3 -c "import json,sys; print(json.load(open(sys.argv[1])).get(sys.argv[2], ''))" "$config_file" "$key"
}

# Main logic
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Check for config file in multiple locations:
# 1. Same directory as this script (for Docker builds)
# 2. Repo root/.github/ (for normal builds)
if [[ -f "$SCRIPT_DIR/build-config.json" ]]; then
    CONFIG_FILE="$SCRIPT_DIR/build-config.json"
elif REPO_ROOT=$(find_repo_root "$SCRIPT_DIR" 2>/dev/null); then
    CONFIG_FILE="$REPO_ROOT/.github/build-config.json"
else
    echo "Error: Could not find build-config.json" >&2
    exit 1
fi

# If called with --get, just print the value and exit
if [[ "${1:-}" == "--get" ]]; then
    if [[ -z "${2:-}" ]]; then
        echo "Usage: $0 --get <key>" >&2
        exit 1
    fi
    get_config_value "$2" "$CONFIG_FILE"
    exit 0
fi

# Export all configuration variables (only if not already set)
export QT_VERSION="${QT_VERSION:-$(get_config_value qt_version "$CONFIG_FILE")}"
export QT_MODULES="${QT_MODULES:-$(get_config_value qt_modules "$CONFIG_FILE")}"
export GST_VERSION="${GST_VERSION:-$(get_config_value gstreamer_version "$CONFIG_FILE")}"
export XCODE_VERSION="${XCODE_VERSION:-$(get_config_value xcode_version "$CONFIG_FILE")}"
export NDK_VERSION="${NDK_VERSION:-$(get_config_value ndk_version "$CONFIG_FILE")}"
export JAVA_VERSION="${JAVA_VERSION:-$(get_config_value java_version "$CONFIG_FILE")}"
export ANDROID_PLATFORM="${ANDROID_PLATFORM:-$(get_config_value android_platform "$CONFIG_FILE")}"
export CCACHE_VERSION="${CCACHE_VERSION:-$(get_config_value ccache_version "$CONFIG_FILE")}"

# Print config if running directly (not sourced)
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    echo "Build Configuration (from $CONFIG_FILE):"
    echo "  QT_VERSION=$QT_VERSION"
    echo "  QT_MODULES=$QT_MODULES"
    echo "  GST_VERSION=$GST_VERSION"
    echo "  XCODE_VERSION=$XCODE_VERSION"
    echo "  NDK_VERSION=$NDK_VERSION"
    echo "  JAVA_VERSION=$JAVA_VERSION"
    echo "  ANDROID_PLATFORM=$ANDROID_PLATFORM"
    echo "  CCACHE_VERSION=$CCACHE_VERSION"
fi

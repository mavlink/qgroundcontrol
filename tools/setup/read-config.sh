#!/usr/bin/env bash
#
# Read build configuration from .github/build-config.json
#
# This script exports version variables from the centralized config file.
# Source this script to get the variables, or call with --get <key> to retrieve a single value.
#
# Usage:
#   source read-config.sh              # Export all variables
#   ./read-config.sh                   # Print all config values
#   ./read-config.sh --get qt_version  # Get single value
#
# Exports:
#   QT_VERSION, QT_MINIMUM_VERSION, QT_MODULES,
#   GSTREAMER_MINIMUM_VERSION, GSTREAMER_MACOS_VERSION, GSTREAMER_ANDROID_VERSION,
#   XCODE_VERSION, XCODE_IOS_VERSION, NDK_VERSION, NDK_FULL_VERSION, JAVA_VERSION,
#   ANDROID_PLATFORM, ANDROID_MIN_SDK, ANDROID_BUILD_TOOLS, ANDROID_CMDLINE_TOOLS,
#   CMAKE_MINIMUM_VERSION

set -euo pipefail

# Resolve the physical path to this script for both bash and zsh shells
_script_source_path() {
    if [[ -n "${BASH_VERSION:-}" ]]; then
        printf '%s\n' "${BASH_SOURCE[0]}"
    elif [[ -n "${ZSH_VERSION:-}" ]]; then
        eval 'printf "%s\\n" "${(%):-%x}"'
    else
        printf '%s\n' "$0"
    fi
}

SCRIPT_SOURCE="$(_script_source_path)"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_SOURCE")" && pwd)"

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

# Get a value from the JSON config
# Uses jq if available, falls back to Python for portability
get_config_value() {
    local key="$1"
    local config_file="$2"
    if command -v jq &> /dev/null; then
        jq -r ".$key // empty" "$config_file"
    elif command -v python3 &> /dev/null; then
        python3 -c "import json,sys; print(json.load(open(sys.argv[1])).get(sys.argv[2], ''))" "$config_file" "$key"
    else
        echo "Error: Either jq or python3 is required" >&2
        exit 1
    fi
}

# Check for config file in multiple locations:
# 1. Same directory as this script (for Docker builds where config is copied)
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
        echo "" >&2
        echo "Available keys:" >&2
        echo "  qt_version, qt_modules, qt_minimum_version," >&2
        echo "  gstreamer_minimum_version, gstreamer_macos_version," >&2
        echo "  gstreamer_android_version, gstreamer_windows_version," >&2
        echo "  ndk_version, ndk_full_version, java_version," >&2
        echo "  android_platform, android_min_sdk, android_build_tools, android_cmdline_tools," >&2
        echo "  xcode_version, xcode_ios_version, cmake_minimum_version" >&2
        exit 1
    fi
    get_config_value "$2" "$CONFIG_FILE"
    exit 0
fi

# Export all configuration variables (only if not already set)
export QT_VERSION="${QT_VERSION:-$(get_config_value qt_version "$CONFIG_FILE")}"
export QT_MINIMUM_VERSION="${QT_MINIMUM_VERSION:-$(get_config_value qt_minimum_version "$CONFIG_FILE")}"
export QT_MODULES="${QT_MODULES:-$(get_config_value qt_modules "$CONFIG_FILE")}"
export GSTREAMER_MINIMUM_VERSION="${GSTREAMER_MINIMUM_VERSION:-$(get_config_value gstreamer_minimum_version "$CONFIG_FILE")}"
export GSTREAMER_MACOS_VERSION="${GSTREAMER_MACOS_VERSION:-$(get_config_value gstreamer_macos_version "$CONFIG_FILE")}"
export GSTREAMER_ANDROID_VERSION="${GSTREAMER_ANDROID_VERSION:-$(get_config_value gstreamer_android_version "$CONFIG_FILE")}"
export XCODE_VERSION="${XCODE_VERSION:-$(get_config_value xcode_version "$CONFIG_FILE")}"
export XCODE_IOS_VERSION="${XCODE_IOS_VERSION:-$(get_config_value xcode_ios_version "$CONFIG_FILE")}"
export NDK_VERSION="${NDK_VERSION:-$(get_config_value ndk_version "$CONFIG_FILE")}"
export NDK_FULL_VERSION="${NDK_FULL_VERSION:-$(get_config_value ndk_full_version "$CONFIG_FILE")}"
export JAVA_VERSION="${JAVA_VERSION:-$(get_config_value java_version "$CONFIG_FILE")}"
export ANDROID_PLATFORM="${ANDROID_PLATFORM:-$(get_config_value android_platform "$CONFIG_FILE")}"
export ANDROID_MIN_SDK="${ANDROID_MIN_SDK:-$(get_config_value android_min_sdk "$CONFIG_FILE")}"
export ANDROID_BUILD_TOOLS="${ANDROID_BUILD_TOOLS:-$(get_config_value android_build_tools "$CONFIG_FILE")}"
export ANDROID_CMDLINE_TOOLS="${ANDROID_CMDLINE_TOOLS:-$(get_config_value android_cmdline_tools "$CONFIG_FILE")}"
export CMAKE_MINIMUM_VERSION="${CMAKE_MINIMUM_VERSION:-$(get_config_value cmake_minimum_version "$CONFIG_FILE")}"

# Print config if running directly (not sourced)
if [[ "$SCRIPT_SOURCE" == "$0" ]]; then
    echo "Build Configuration (from $CONFIG_FILE):"
    echo "  QT_VERSION=$QT_VERSION"
    echo "  QT_MINIMUM_VERSION=$QT_MINIMUM_VERSION"
    echo "  QT_MODULES=$QT_MODULES"
    echo "  GSTREAMER_MINIMUM_VERSION=$GSTREAMER_MINIMUM_VERSION"
    echo "  GSTREAMER_MACOS_VERSION=$GSTREAMER_MACOS_VERSION"
    echo "  GSTREAMER_ANDROID_VERSION=$GSTREAMER_ANDROID_VERSION"
    echo "  XCODE_VERSION=$XCODE_VERSION"
    echo "  XCODE_IOS_VERSION=$XCODE_IOS_VERSION"
    echo "  NDK_VERSION=$NDK_VERSION"
    echo "  NDK_FULL_VERSION=$NDK_FULL_VERSION"
    echo "  JAVA_VERSION=$JAVA_VERSION"
    echo "  ANDROID_PLATFORM=$ANDROID_PLATFORM"
    echo "  ANDROID_MIN_SDK=$ANDROID_MIN_SDK"
    echo "  ANDROID_BUILD_TOOLS=$ANDROID_BUILD_TOOLS"
    echo "  ANDROID_CMDLINE_TOOLS=$ANDROID_CMDLINE_TOOLS"
    echo "  CMAKE_MINIMUM_VERSION=$CMAKE_MINIMUM_VERSION"
fi

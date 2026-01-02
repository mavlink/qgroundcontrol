#!/usr/bin/env bash
#
# Install Qt and Android development environment
#
# DEPRECATION NOTICE: This script is a thin wrapper around install-qt.py.
# For new development, use install-qt.py directly:
#   python3 tools/setup/install-qt.py --target android --install-java --install-android-sdk
#
# This script installs:
#   - Java JDK (via package manager or Homebrew)
#   - Android command-line tools
#   - Android SDK (platform-tools, platforms, build-tools)
#   - Android NDK
#   - Qt for host platform (Linux/macOS)
#   - Qt for Android ABIs (arm64-v8a, armeabi-v7a, x86_64, x86)
#
# Environment variables (all optional, defaults from build-config.json):
#   QT_VERSION          - Qt version to install
#   QT_PATH             - Qt installation prefix (default: /opt/Qt)
#   QT_MODULES          - Space-separated Qt modules
#   ANDROID_SDK_ROOT    - Android SDK location (default: ~/Android/Sdk)
#   ANDROID_ABIS        - ABIs to install (default: "arm64-v8a armeabi-v7a")
#   JAVA_VERSION        - Java version (from build-config.json)
#   NDK_FULL_VERSION    - NDK version (from build-config.json)
#   ANDROID_PLATFORM    - Android API level (from build-config.json)
#   ANDROID_BUILD_TOOLS - Build tools version (from build-config.json)
#
# Usage:
#   ./install-qt-android.sh
#   ANDROID_ABIS="arm64-v8a" ./install-qt-android.sh  # Single ABI
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PYTHON_SCRIPT="$SCRIPT_DIR/install-qt.py"

# Detect OS
OS="$(uname -s)"
case "$OS" in
    Linux)  HOST_OS="linux" ;;
    Darwin) HOST_OS="mac" ;;
    *)      echo "Error: Unsupported OS: $OS" >&2; exit 1 ;;
esac

# Ensure Python 3 is available
if ! command -v python3 &> /dev/null; then
    echo "Installing Python 3..."
    if [[ "$HOST_OS" == "linux" ]]; then
        if command -v apt-get &> /dev/null; then
            sudo apt-get update -y --quiet
            sudo apt-get install -y python3 python3-pip
        elif command -v dnf &> /dev/null; then
            sudo dnf install -y python3 python3-pip
        else
            echo "Error: Python 3 required but could not install" >&2
            exit 1
        fi
    else
        brew install python3
    fi
fi

# Build arguments for Python script
ARGS=()
ARGS+=(--target android)
ARGS+=(--host "$HOST_OS")
ARGS+=(--install-aqt)
ARGS+=(--install-java)
ARGS+=(--install-android-sdk)

[[ -n "${QT_VERSION:-}" ]] && ARGS+=(--version "$QT_VERSION")
[[ -n "${QT_PATH:-}" ]] && ARGS+=(--path "$QT_PATH")
[[ -n "${QT_MODULES:-}" ]] && ARGS+=(--modules "$QT_MODULES")
[[ -n "${ANDROID_ABIS:-}" ]] && ARGS+=(--android-abis "$ANDROID_ABIS")
[[ -n "${ANDROID_SDK_ROOT:-}" ]] && ARGS+=(--android-sdk-root "$ANDROID_SDK_ROOT")
[[ -n "${JAVA_VERSION:-}" ]] && ARGS+=(--java-version "$JAVA_VERSION")
[[ -n "${NDK_FULL_VERSION:-}" ]] && ARGS+=(--ndk-version "$NDK_FULL_VERSION")
[[ -n "${ANDROID_PLATFORM:-}" ]] && ARGS+=(--android-platform "$ANDROID_PLATFORM")
[[ -n "${ANDROID_BUILD_TOOLS:-}" ]] && ARGS+=(--android-build-tools "$ANDROID_BUILD_TOOLS")
[[ -n "${ANDROID_CMDLINE_TOOLS:-}" ]] && ARGS+=(--android-cmdline-tools "$ANDROID_CMDLINE_TOOLS")

# Run Python script
exec python3 "$PYTHON_SCRIPT" "${ARGS[@]}"

#!/bin/bash
#
# Build GStreamer from source for iOS using Cerbero
#
# This script cross-compiles GStreamer for iOS with plugins needed for
# QGC video streaming. Must be run from macOS with Xcode installed.
#
# Usage: ./build-gstreamer-ios.sh [OPTIONS]
#
# Options:
#   -v, --version VERSION    GStreamer version (default: from build-config.json)
#   -a, --arch ARCH          Target: arm64, x86_64, or universal (default: universal)
#   -t, --type TYPE          Build type: release or debug (default: release)
#   -w, --work-dir DIR       Working directory (default: /tmp/gst-ios)
#   -o, --output DIR         Output directory for packages (default: ./gstreamer-ios)
#   -j, --jobs N             Parallel jobs (default: auto-detect)
#   -c, --clean              Clean build before starting
#   --simulator              Build for iOS Simulator (x86_64/arm64 sim)
#   -h, --help               Show this help message
#
# Requirements:
#   - macOS with Xcode and Command Line Tools
#   - Python 3.8+
#   - Git
#   - iOS SDK (included with Xcode)
#
# Examples:
#   ./build-gstreamer-ios.sh                              # Universal device build
#   ./build-gstreamer-ios.sh -a arm64                     # ARM64 device only
#   ./build-gstreamer-ios.sh --simulator                  # Simulator build

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Source centralized config
if [[ -f "$SCRIPT_DIR/../read-config.sh" ]]; then
    # shellcheck source=read-config.sh
    source "$SCRIPT_DIR/../read-config.sh"
fi

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Defaults
ARCH="universal"
BUILD_TYPE="release"
WORK_DIR="/tmp/gst-ios"
OUTPUT_DIR="$SCRIPT_DIR/../../gstreamer-ios"
JOBS=""
CLEAN=false
SIMULATOR=false

log_info()  { echo -e "${BLUE}[INFO]${NC} $*"; }
log_ok()    { echo -e "${GREEN}[OK]${NC} $*"; }
log_warn()  { echo -e "${YELLOW}[WARN]${NC} $*"; }
log_error() { echo -e "${RED}[ERROR]${NC} $*" >&2; }

show_help() {
    head -32 "$0" | tail -28
    exit 0
}

check_macos() {
    if [[ "$(uname)" != "Darwin" ]]; then
        log_error "iOS builds require macOS. Current OS: $(uname)"
        exit 1
    fi
}

check_dependencies() {
    local missing=()

    for cmd in git python3 xcodebuild; do
        if ! command -v "$cmd" &> /dev/null; then
            missing+=("$cmd")
        fi
    done

    if [[ ${#missing[@]} -gt 0 ]]; then
        log_error "Missing required tools: ${missing[*]}"
        if [[ " ${missing[*]} " =~ "xcodebuild" ]]; then
            log_info "Install Xcode from the App Store"
        fi
        exit 1
    fi

    # Check for Xcode command line tools
    if ! xcode-select -p &> /dev/null; then
        log_error "Xcode command line tools not installed"
        log_info "Install with: xcode-select --install"
        exit 1
    fi

    # Check Python version
    local py_version
    py_version=$(python3 -c 'import sys; print(f"{sys.version_info.major}.{sys.version_info.minor}")')
    local py_major py_minor
    py_major=$(echo "$py_version" | cut -d. -f1)
    py_minor=$(echo "$py_version" | cut -d. -f2)
    if [[ "$py_major" -lt 3 ]] || [[ "$py_major" -eq 3 && "$py_minor" -lt 8 ]]; then
        log_error "Python 3.8+ required, found $py_version"
        exit 1
    fi

    # Check for iOS SDK
    if ! xcrun --sdk iphoneos --show-sdk-path &> /dev/null; then
        log_error "iOS SDK not found. Install Xcode with iOS SDK."
        exit 1
    fi
}

detect_jobs() {
    if [[ -n "$JOBS" ]]; then
        echo "$JOBS"
    else
        sysctl -n hw.ncpu 2>/dev/null || echo 4
    fi
}

get_cerbero_config() {
    local arch="$1"
    local sim="$2"

    if [[ "$sim" == true ]]; then
        case "$arch" in
            arm64)     echo "cross-ios-arm64-simulator.cbc" ;;
            x86_64)    echo "cross-ios-x86-64-simulator.cbc" ;;
            universal) echo "cross-ios-universal-simulator.cbc" ;;
            *)
                log_error "Unknown simulator architecture: $arch"
                exit 1
                ;;
        esac
    else
        case "$arch" in
            arm64)     echo "cross-ios-arm64.cbc" ;;
            x86_64)    echo "cross-ios-x86-64.cbc" ;;  # Legacy, rarely used
            universal) echo "cross-ios-universal.cbc" ;;
            *)
                log_error "Unknown architecture: $arch"
                exit 1
                ;;
        esac
    fi
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -v|--version)
            GST_VERSION="$2"
            shift 2
            ;;
        -a|--arch)
            ARCH="$2"
            shift 2
            ;;
        -t|--type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -w|--work-dir)
            WORK_DIR="$2"
            shift 2
            ;;
        -o|--output)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        -j|--jobs)
            JOBS="$2"
            shift 2
            ;;
        -c|--clean)
            CLEAN=true
            shift
            ;;
        --simulator)
            SIMULATOR=true
            shift
            ;;
        -h|--help)
            show_help
            ;;
        *)
            log_error "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Check we're on macOS
check_macos

# Use environment variable or default version
if [[ -z "${GST_VERSION:-}" ]]; then
    GST_VERSION="${GSTREAMER_MACOS_VERSION:-1.24.0}"
fi

# Validate inputs
if [[ "$BUILD_TYPE" != "release" && "$BUILD_TYPE" != "debug" ]]; then
    log_error "Invalid build type: $BUILD_TYPE"
    exit 1
fi

CERBERO_CONFIG=$(get_cerbero_config "$ARCH" "$SIMULATOR")
NUM_JOBS=$(detect_jobs)
CERBERO_DIR="$WORK_DIR/cerbero"

# Determine target description
TARGET_DESC="iOS Device"
if [[ "$SIMULATOR" == true ]]; then
    TARGET_DESC="iOS Simulator"
fi

echo ""
log_info "Building GStreamer $GST_VERSION for $TARGET_DESC"
echo "  Architecture:   $ARCH"
echo "  Config:         $CERBERO_CONFIG"
echo "  Build type:     $BUILD_TYPE"
echo "  Parallel jobs:  $NUM_JOBS"
echo "  Work dir:       $WORK_DIR"
echo "  Output dir:     $OUTPUT_DIR"
echo "  Xcode:          $(xcodebuild -version | head -1)"
echo "  iOS SDK:        $(xcrun --sdk iphoneos --show-sdk-version)"
echo ""

# Check dependencies
log_info "Checking dependencies..."
check_dependencies
log_ok "Dependencies verified"

# Clean if requested
if [[ "$CLEAN" == true && -d "$WORK_DIR" ]]; then
    log_info "Cleaning previous build..."
    rm -rf "$WORK_DIR"
fi

# Create directories
mkdir -p "$WORK_DIR" "$OUTPUT_DIR"

# Clone or update Cerbero
if [[ ! -d "$CERBERO_DIR" ]]; then
    log_info "Cloning Cerbero..."
    git clone --depth 1 --branch "$GST_VERSION" \
        https://github.com/GStreamer/cerbero.git "$CERBERO_DIR"
else
    log_info "Using existing Cerbero at $CERBERO_DIR"
fi

cd "$CERBERO_DIR"

# Create custom cerbero config for QGC
CUSTOM_CONFIG="$CERBERO_DIR/qgc-ios.cbc"
cat > "$CUSTOM_CONFIG" << EOF
# QGroundControl custom Cerbero configuration for iOS
# Enables parallel builds

allow_parallel_build = True
num_of_cpus = $NUM_JOBS

# Build variant
variants = ['nodebug']
EOF

# Set debug variant if debug build
if [[ "$BUILD_TYPE" == "debug" ]]; then
    sed -i '' "s/nodebug/debug/" "$CUSTOM_CONFIG"
fi

# Bootstrap Cerbero
log_info "Bootstrapping Cerbero for iOS..."
./cerbero-uninstalled -c "config/$CERBERO_CONFIG" bootstrap

# Build GStreamer
log_info "Building GStreamer (this will take a while)..."
./cerbero-uninstalled -c "config/$CERBERO_CONFIG" build gstreamer-1.0

# Package - creates .pkg files
log_info "Creating package..."
./cerbero-uninstalled -c "config/$CERBERO_CONFIG" package gstreamer-1.0

# Copy and rename output to consistent naming convention
# Format: gstreamer-1.0-ios-{arch}-{version}.tar.xz
log_info "Copying packages to $OUTPUT_DIR..."

TARGET_SUFFIX="$ARCH"
if [[ "$SIMULATOR" == true ]]; then
    TARGET_SUFFIX="$ARCH-simulator"
fi
ARCHIVE_NAME="gstreamer-1.0-ios-$TARGET_SUFFIX-$GST_VERSION"

# Look for Cerbero output and rename
CERBERO_PKG=$(find . -maxdepth 1 -name "gstreamer-1.0-ios-*.tar.xz" | head -1)
if [[ -n "$CERBERO_PKG" ]]; then
    cp "$CERBERO_PKG" "$OUTPUT_DIR/$ARCHIVE_NAME.tar.xz"
else
    # Fallback: copy any packages
    find . -maxdepth 1 \( -name "*.pkg" -o -name "*.tar.*" -o -name "*.framework.zip" \) -exec cp {} "$OUTPUT_DIR/" \;
fi

# List created packages
echo ""
log_ok "GStreamer $GST_VERSION for $TARGET_DESC built successfully!"
echo ""
echo "Packages created in $OUTPUT_DIR:"
ls -la "$OUTPUT_DIR"/*.tar.* "$OUTPUT_DIR"/*.pkg "$OUTPUT_DIR"/*.framework.zip 2>/dev/null || echo "  (no packages found)"
echo ""
echo "Archive: $ARCHIVE_NAME.tar.xz"
echo ""
echo "To use with QGC iOS build:"
echo "  1. Extract the archive or install the .pkg files"
echo "  2. The GStreamer framework will be at /Library/Frameworks/GStreamer.framework"
echo "  3. Add the framework to your Xcode project"
echo ""

# Output for CI
if [[ -n "${GITHUB_OUTPUT:-}" ]]; then
    echo "gstreamer_output=$OUTPUT_DIR" >> "$GITHUB_OUTPUT"
    echo "gstreamer_version=$GST_VERSION" >> "$GITHUB_OUTPUT"
    echo "gstreamer_arch=$ARCH" >> "$GITHUB_OUTPUT"
    echo "gstreamer_simulator=$SIMULATOR" >> "$GITHUB_OUTPUT"
    echo "archive_name=$ARCHIVE_NAME" >> "$GITHUB_OUTPUT"
    echo "archive_path=$OUTPUT_DIR/$ARCHIVE_NAME.tar.xz" >> "$GITHUB_OUTPUT"
fi

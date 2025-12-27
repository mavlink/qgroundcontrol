#!/bin/bash
#
# Build GStreamer from source for Android using Cerbero
#
# This script cross-compiles GStreamer for Android with plugins needed for
# QGC video streaming. Can be run from Linux, macOS, or Windows (WSL/MSYS2).
#
# Usage: ./build-gstreamer-android.sh [OPTIONS]
#
# Options:
#   -v, --version VERSION    GStreamer version (default: from build-config.json)
#   -a, --arch ARCH          Target: arm64, armv7, x86, x86_64, or universal (default: universal)
#   -t, --type TYPE          Build type: release or debug (default: release)
#   -w, --work-dir DIR       Working directory (default: /tmp/gst-android)
#   -o, --output DIR         Output directory for packages (default: ./gstreamer-android)
#   -j, --jobs N             Parallel jobs (default: auto-detect)
#   -c, --clean              Clean build before starting
#   -h, --help               Show this help message
#
# Requirements:
#   - Python 3.8+
#   - Git
#   - Android NDK (auto-downloaded if not set via ANDROID_NDK_ROOT)
#
# Examples:
#   ./build-gstreamer-android.sh                          # Build universal (all ABIs)
#   ./build-gstreamer-android.sh -a arm64                 # ARM64 only
#   ./build-gstreamer-android.sh -a armv7 -t debug        # ARMv7 debug build

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
WORK_DIR="/tmp/gst-android"
OUTPUT_DIR="$SCRIPT_DIR/../../gstreamer-android"
JOBS=""
CLEAN=false

log_info()  { echo -e "${BLUE}[INFO]${NC} $*"; }
log_ok()    { echo -e "${GREEN}[OK]${NC} $*"; }
log_warn()  { echo -e "${YELLOW}[WARN]${NC} $*"; }
log_error() { echo -e "${RED}[ERROR]${NC} $*" >&2; }

show_help() {
    head -30 "$0" | tail -26
    exit 0
}

check_dependencies() {
    local missing=()

    for cmd in git python3; do
        if ! command -v "$cmd" &> /dev/null; then
            missing+=("$cmd")
        fi
    done

    if [[ ${#missing[@]} -gt 0 ]]; then
        log_error "Missing required tools: ${missing[*]}"
        exit 1
    fi

    # Check Python version (3.8+ required)
    local py_major py_minor
    py_major=$(python3 -c 'import sys; print(sys.version_info.major)')
    py_minor=$(python3 -c 'import sys; print(sys.version_info.minor)')
    if [[ "$py_major" -lt 3 ]] || [[ "$py_major" -eq 3 && "$py_minor" -lt 8 ]]; then
        log_error "Python 3.8+ required, found $py_major.$py_minor"
        exit 1
    fi
}

detect_jobs() {
    if [[ -n "$JOBS" ]]; then
        echo "$JOBS"
    elif [[ -f /proc/cpuinfo ]]; then
        grep -c ^processor /proc/cpuinfo
    elif command -v sysctl &> /dev/null; then
        sysctl -n hw.ncpu 2>/dev/null || echo 4
    else
        echo 4
    fi
}

get_cerbero_config() {
    local arch="$1"
    case "$arch" in
        arm64)   echo "cross-android-arm64.cbc" ;;
        armv7)   echo "cross-android-armv7.cbc" ;;
        x86)     echo "cross-android-x86.cbc" ;;
        x86_64)  echo "cross-android-x86-64.cbc" ;;
        universal) echo "cross-android-universal.cbc" ;;
        *)
            log_error "Unknown architecture: $arch"
            exit 1
            ;;
    esac
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
        -h|--help)
            show_help
            ;;
        *)
            log_error "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Use environment variable or default version
if [[ -z "${GST_VERSION:-}" ]]; then
    GST_VERSION="${GSTREAMER_ANDROID_VERSION:-1.24.0}"
fi

# Validate inputs
if [[ "$BUILD_TYPE" != "release" && "$BUILD_TYPE" != "debug" ]]; then
    log_error "Invalid build type: $BUILD_TYPE"
    exit 1
fi

CERBERO_CONFIG=$(get_cerbero_config "$ARCH")
NUM_JOBS=$(detect_jobs)
CERBERO_DIR="$WORK_DIR/cerbero"

echo ""
log_info "Building GStreamer $GST_VERSION for Android"
echo "  Architecture:   $ARCH"
echo "  Config:         $CERBERO_CONFIG"
echo "  Build type:     $BUILD_TYPE"
echo "  Parallel jobs:  $NUM_JOBS"
echo "  Work dir:       $WORK_DIR"
echo "  Output dir:     $OUTPUT_DIR"
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
CUSTOM_CONFIG="$CERBERO_DIR/qgc-android.cbc"
cat > "$CUSTOM_CONFIG" << 'EOF'
# QGroundControl custom Cerbero configuration for Android
# Enables parallel builds and minimal plugin set

allow_parallel_build = True
num_of_cpus = NUM_JOBS_PLACEHOLDER

# Only build what QGC needs
# Core plugins for video streaming
variants = ['nodebug']
EOF

# Replace placeholder with actual jobs
sed -i.bak "s/NUM_JOBS_PLACEHOLDER/$NUM_JOBS/" "$CUSTOM_CONFIG"
rm -f "$CUSTOM_CONFIG.bak"

# Append debug variant if debug build
if [[ "$BUILD_TYPE" == "debug" ]]; then
    sed -i.bak "s/nodebug/debug/" "$CUSTOM_CONFIG"
    rm -f "$CUSTOM_CONFIG.bak"
fi

# Bootstrap Cerbero (installs dependencies and NDK if needed)
log_info "Bootstrapping Cerbero for Android..."
./cerbero-uninstalled -c "config/$CERBERO_CONFIG" bootstrap

# Build GStreamer
log_info "Building GStreamer (this will take a while)..."
./cerbero-uninstalled -c "config/$CERBERO_CONFIG" build gstreamer-1.0

# Package
log_info "Creating package..."
./cerbero-uninstalled -c "config/$CERBERO_CONFIG" package gstreamer-1.0

# Copy and rename output to consistent naming convention
# Format: gstreamer-1.0-android-{arch}-{version}.tar.xz
log_info "Copying packages to $OUTPUT_DIR..."

ARCHIVE_NAME="gstreamer-1.0-android-$ARCH-$GST_VERSION"

# Look for Cerbero output packages and rename to consistent format
CERBERO_PKG=$(find . -maxdepth 1 -name "gstreamer-1.0-android-*.tar.xz" | head -1)
if [[ -n "$CERBERO_PKG" ]]; then
    cp "$CERBERO_PKG" "$OUTPUT_DIR/$ARCHIVE_NAME.tar.xz"
else
    # Fallback: copy any tar files
    find . -maxdepth 1 -name "*.tar.*" -exec cp {} "$OUTPUT_DIR/" \;
fi

# List created packages
echo ""
log_ok "GStreamer $GST_VERSION for Android built successfully!"
echo ""
echo "Packages created in $OUTPUT_DIR:"
ls -la "$OUTPUT_DIR"/*.tar.* 2>/dev/null || echo "  (no packages found)"
echo ""
echo "Archive: $ARCHIVE_NAME.tar.xz"
echo ""
echo "To use with QGC Android build:"
echo "  1. Extract the package to a known location"
echo "  2. Set GSTREAMER_ROOT_ANDROID to that location"
echo "  3. Configure CMake with -DGStreamer_ROOT_DIR=\$GSTREAMER_ROOT_ANDROID"
echo ""

# Output for CI
if [[ -n "${GITHUB_OUTPUT:-}" ]]; then
    echo "gstreamer_output=$OUTPUT_DIR" >> "$GITHUB_OUTPUT"
    echo "gstreamer_version=$GST_VERSION" >> "$GITHUB_OUTPUT"
    echo "gstreamer_arch=$ARCH" >> "$GITHUB_OUTPUT"
    echo "archive_name=$ARCHIVE_NAME" >> "$GITHUB_OUTPUT"
    echo "archive_path=$OUTPUT_DIR/$ARCHIVE_NAME.tar.xz" >> "$GITHUB_OUTPUT"
fi

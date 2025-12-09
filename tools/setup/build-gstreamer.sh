#!/bin/bash
#
# Build GStreamer from source for QGroundControl
#
# This script builds a minimal GStreamer with only the plugins needed for
# QGC video streaming: RTSP, RTP, UDP, H.264/H.265, MPEG-TS, and Qt6 integration.
#
# Usage: ./build-gstreamer.sh [OPTIONS]
#
# Options:
#   -v, --version VERSION    GStreamer version (default: from build-config.json)
#   -t, --type TYPE          Build type: release or debug (default: release)
#   -w, --work-dir DIR       Working directory for source (default: /tmp)
#   -p, --prefix DIR         Install prefix (default: /tmp/gst)
#   -j, --jobs N             Parallel jobs (default: auto-detect)
#   -c, --clean              Clean build directory before building
#   -h, --help               Show this help message
#
# Examples:
#   ./build-gstreamer.sh                           # Build with defaults
#   ./build-gstreamer.sh -p /opt/gstreamer         # Custom install location
#   ./build-gstreamer.sh -t debug -j 4             # Debug build with 4 jobs
#   ./build-gstreamer.sh --clean                   # Clean rebuild

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Source centralized config (sets GST_VERSION if not already set)
# shellcheck source=read-config.sh
source "$SCRIPT_DIR/read-config.sh"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Defaults (GST_VERSION now comes from read-config.sh)
BUILD_TYPE="release"
WORK_DIR="/tmp"
INSTALL_PREFIX="/tmp/gst"
JOBS=""
CLEAN=false

log_info()  { echo -e "${BLUE}[INFO]${NC} $*"; }
log_ok()    { echo -e "${GREEN}[OK]${NC} $*"; }
log_warn()  { echo -e "${YELLOW}[WARN]${NC} $*"; }
log_error() { echo -e "${RED}[ERROR]${NC} $*" >&2; }

show_help() {
    head -26 "$0" | tail -22
    exit 0
}

check_dependencies() {
    local missing=()

    for cmd in git python3 pkg-config; do
        if ! command -v "$cmd" &> /dev/null; then
            missing+=("$cmd")
        fi
    done

    if [[ ${#missing[@]} -gt 0 ]]; then
        log_error "Missing required tools: ${missing[*]}"
        log_info "Install with: sudo apt install git python3 pkg-config"
        exit 1
    fi
}

detect_arch() {
    local arch
    arch=$(uname -m)
    case "$arch" in
        x86_64)  echo "x86_64-linux-gnu" ;;
        aarch64) echo "aarch64-linux-gnu" ;;
        armv7l)  echo "arm-linux-gnueabihf" ;;
        *)       echo "$arch-linux-gnu" ;;
    esac
}

detect_jobs() {
    if [[ -n "$JOBS" ]]; then
        echo "$JOBS"
    elif [[ -f /proc/cpuinfo ]]; then
        grep -c ^processor /proc/cpuinfo
    else
        echo 4
    fi
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -v|--version)
            GST_VERSION="$2"
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
        -p|--prefix)
            INSTALL_PREFIX="$2"
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

# Validate build type
if [[ "$BUILD_TYPE" != "release" && "$BUILD_TYPE" != "debug" ]]; then
    log_error "Invalid build type: $BUILD_TYPE (must be 'release' or 'debug')"
    exit 1
fi

ARCH=$(detect_arch)
NUM_JOBS=$(detect_jobs)
SOURCE_DIR="$WORK_DIR/gstreamer"
BUILD_DIR="$SOURCE_DIR/builddir"

echo ""
log_info "Building GStreamer $GST_VERSION for QGroundControl"
echo "  Build type:    $BUILD_TYPE"
echo "  Architecture:  $ARCH"
echo "  Parallel jobs: $NUM_JOBS"
echo "  Source dir:    $SOURCE_DIR"
echo "  Install prefix: $INSTALL_PREFIX"
echo ""

# Check dependencies
log_info "Checking dependencies..."
check_dependencies
log_ok "All dependencies found"

# Install Python build tools
log_info "Installing meson and ninja..."
python3 -m pip install --user --quiet ninja meson

# Ensure meson/ninja are in PATH
export PATH="$HOME/.local/bin:$PATH"

# Clean if requested
if [[ "$CLEAN" == true && -d "$SOURCE_DIR" ]]; then
    log_info "Cleaning previous build..."
    rm -rf "$SOURCE_DIR"
fi

# Clone GStreamer
if [[ ! -d "$SOURCE_DIR" ]]; then
    log_info "Cloning GStreamer $GST_VERSION..."
    mkdir -p "$WORK_DIR"
    git clone --depth 1 --branch "$GST_VERSION" \
        https://github.com/GStreamer/gstreamer.git "$SOURCE_DIR"
else
    log_info "Using existing source at $SOURCE_DIR"
fi

cd "$SOURCE_DIR"

# Configure if needed
if [[ ! -f "$BUILD_DIR/build.ninja" || "$CLEAN" == true ]]; then
    log_info "Configuring GStreamer..."

    # Remove old build dir if exists
    [[ -d "$BUILD_DIR" ]] && rm -rf "$BUILD_DIR"

    meson setup "$BUILD_DIR" \
        --prefix="$INSTALL_PREFIX" \
        --buildtype="$BUILD_TYPE" \
        --wrap-mode=forcefallback \
        --strip \
        -Dauto_features=disabled \
        -Dgst-full-libraries=video,gl \
        -Dgpl=enabled \
        -Dlibav=enabled \
        -Dorc=enabled \
        -Dqt6=enabled \
        -Dvaapi=enabled \
        -Dbase=enabled \
        -Dgst-plugins-base:app=enabled \
        -Dgst-plugins-base:gl=enabled \
        -Dgst-plugins-base:gl_api=opengl,gles2 \
        -Dgst-plugins-base:gl_platform=glx,egl \
        -Dgst-plugins-base:gl_winsys=x11,egl,wayland \
        -Dgst-plugins-base:playback=enabled \
        -Dgst-plugins-base:tcp=enabled \
        -Dgst-plugins-base:x11=enabled \
        -Dgood=enabled \
        -Dgst-plugins-good:isomp4=enabled \
        -Dgst-plugins-good:matroska=enabled \
        -Dgst-plugins-good:qt-egl=enabled \
        -Dgst-plugins-good:qt-method=auto \
        -Dgst-plugins-good:qt-wayland=enabled \
        -Dgst-plugins-good:qt-x11=enabled \
        -Dgst-plugins-good:qt6=enabled \
        -Dgst-plugins-good:rtp=enabled \
        -Dgst-plugins-good:rtpmanager=enabled \
        -Dgst-plugins-good:rtsp=enabled \
        -Dgst-plugins-good:udp=enabled \
        -Dbad=enabled \
        -Dgst-plugins-bad:gl=enabled \
        -Dgst-plugins-bad:mpegtsdemux=enabled \
        -Dgst-plugins-bad:rtp=enabled \
        -Dgst-plugins-bad:sdp=enabled \
        -Dgst-plugins-bad:va=enabled \
        -Dgst-plugins-bad:videoparsers=enabled \
        -Dgst-plugins-bad:wayland=enabled \
        -Dgst-plugins-bad:x11=enabled \
        -Dgst-plugins-bad:x265=enabled \
        -Dugly=enabled \
        -Dgst-plugins-ugly:x264=enabled
else
    log_info "Using existing configuration"
fi

# Build
log_info "Compiling GStreamer (this may take a while)..."
meson compile -C "$BUILD_DIR" -j "$NUM_JOBS"

# Install
log_info "Installing GStreamer..."
meson install -C "$BUILD_DIR"

# Success
echo ""
log_ok "GStreamer $GST_VERSION installed successfully!"
echo ""
echo "To use with QGroundControl, set:"
echo "  export PKG_CONFIG_PATH=\"$INSTALL_PREFIX/lib/$ARCH/pkgconfig:\$PKG_CONFIG_PATH\""
echo "  export GST_PLUGIN_PATH=\"$INSTALL_PREFIX/lib/$ARCH/gstreamer-1.0\""
echo ""

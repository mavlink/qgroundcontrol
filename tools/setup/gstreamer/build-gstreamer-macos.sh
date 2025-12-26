#!/bin/bash
#
# Build GStreamer from source for QGroundControl on macOS
#
# This script builds a minimal GStreamer with only the plugins needed for
# QGC video streaming: RTSP, RTP, UDP, H.264/H.265, MPEG-TS, and Qt6 integration.
#
# Usage: ./build-gstreamer-macos.sh [OPTIONS]
#
# Options:
#   -v, --version VERSION    GStreamer version (default: from build-config.json)
#   -t, --type TYPE          Build type: release or debug (default: release)
#   -a, --arch ARCH          Architecture: x86_64, arm64, or universal (default: universal)
#   -w, --work-dir DIR       Working directory for source (default: /tmp)
#   -p, --prefix DIR         Install prefix (default: /tmp/gst-macos)
#   -j, --jobs N             Parallel jobs (default: auto-detect)
#   -c, --clean              Clean build directory before building
#   -q, --qt-prefix DIR      Path to Qt6 installation (for qml6 plugin)
#   -h, --help               Show this help message
#
# Examples:
#   ./build-gstreamer-macos.sh                              # Universal build
#   ./build-gstreamer-macos.sh -a arm64                     # ARM64 only
#   ./build-gstreamer-macos.sh -p /opt/gstreamer --clean    # Custom prefix, clean rebuild

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Source centralized config (sets GST_VERSION if not already set)
# shellcheck source=read-config.sh
source "$SCRIPT_DIR/../read-config.sh"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Defaults
BUILD_TYPE="release"
ARCH="universal"
WORK_DIR="/tmp"
INSTALL_PREFIX=""
JOBS=""
CLEAN=false
QT_PREFIX=""

log_info()  { echo -e "${BLUE}[INFO]${NC} $*"; }
log_ok()    { echo -e "${GREEN}[OK]${NC} $*"; }
log_warn()  { echo -e "${YELLOW}[WARN]${NC} $*"; }
log_error() { echo -e "${RED}[ERROR]${NC} $*" >&2; }

show_help() {
    head -28 "$0" | tail -24
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
        log_info "Install with: brew install git python3 pkg-config"
        exit 1
    fi

    # Check for Xcode command line tools
    if ! xcode-select -p &> /dev/null; then
        log_error "Xcode command line tools not installed"
        log_info "Install with: xcode-select --install"
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
        -a|--arch)
            ARCH="$2"
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
        -q|--qt-prefix)
            QT_PREFIX="$2"
            shift 2
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

# Validate architecture
if [[ "$ARCH" != "x86_64" && "$ARCH" != "arm64" && "$ARCH" != "universal" ]]; then
    log_error "Invalid architecture: $ARCH (must be 'x86_64', 'arm64', or 'universal')"
    exit 1
fi

# Set default prefix based on architecture
if [[ -z "$INSTALL_PREFIX" ]]; then
    INSTALL_PREFIX="/tmp/gst-macos-$ARCH"
fi

NUM_JOBS=$(detect_jobs)
SOURCE_DIR="$WORK_DIR/gstreamer"

echo ""
log_info "Building GStreamer $GST_VERSION for QGroundControl (macOS)"
echo "  Build type:     $BUILD_TYPE"
echo "  Architecture:   $ARCH"
echo "  Parallel jobs:  $NUM_JOBS"
echo "  Source dir:     $SOURCE_DIR"
echo "  Install prefix: $INSTALL_PREFIX"
if [[ -n "$QT_PREFIX" ]]; then
    echo "  Qt prefix:      $QT_PREFIX"
fi
echo ""

# Check dependencies
log_info "Checking dependencies..."
check_dependencies
log_ok "All dependencies found"

# Install Python build tools (skip if already available, e.g., from CI venv)
if ! command -v meson &> /dev/null; then
    log_info "Installing meson and ninja..."
    python3 -m pip install --user --quiet --upgrade pip
    python3 -m pip install --user --quiet ninja meson
    export PATH="$HOME/Library/Python/$(python3 -c 'import sys; print(f"{sys.version_info.major}.{sys.version_info.minor}")')/bin:$PATH"
else
    log_info "Using existing meson: $(command -v meson)"
fi

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

# Function to build for a single architecture
build_arch() {
    local target_arch="$1"
    local build_dir="$SOURCE_DIR/builddir-$target_arch"
    local prefix="$INSTALL_PREFIX"

    if [[ "$ARCH" == "universal" ]]; then
        prefix="$INSTALL_PREFIX-$target_arch"
    fi

    log_info "Building for $target_arch..."

    # Configure if needed
    if [[ ! -f "$build_dir/build.ninja" || "$CLEAN" == true ]]; then
        log_info "Configuring GStreamer for $target_arch..."

        [[ -d "$build_dir" ]] && rm -rf "$build_dir"

        # Set up cross-compilation for non-native arch
        local cross_args=()
        local native_arch
        native_arch=$(uname -m)

        if [[ "$target_arch" != "$native_arch" ]]; then
            # Create cross file
            local cross_file="$SOURCE_DIR/cross-$target_arch.txt"
            cat > "$cross_file" << EOF
[binaries]
c = 'clang'
cpp = 'clang++'
objc = 'clang'
objcpp = 'clang++'
ar = 'ar'
strip = 'strip'

[built-in options]
c_args = ['-arch', '$target_arch']
cpp_args = ['-arch', '$target_arch']
objc_args = ['-arch', '$target_arch']
objcpp_args = ['-arch', '$target_arch']
c_link_args = ['-arch', '$target_arch']
cpp_link_args = ['-arch', '$target_arch']

[host_machine]
system = 'darwin'
cpu_family = '$([ "$target_arch" = "arm64" ] && echo "aarch64" || echo "x86_64")'
cpu = '$target_arch'
endian = 'little'
EOF
            cross_args+=(--cross-file "$cross_file")
        fi

        # Qt6 setup
        local qt_args=()
        if [[ -n "$QT_PREFIX" ]]; then
            export PKG_CONFIG_PATH="$QT_PREFIX/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
            export CMAKE_PREFIX_PATH="$QT_PREFIX:${CMAKE_PREFIX_PATH:-}"
            qt_args+=(-Dqt6=enabled)
            qt_args+=(-Dgst-plugins-good:qt6=enabled)
            qt_args+=(-Dgst-plugins-good:qt-method=auto)
        else
            qt_args+=(-Dqt6=disabled)
            qt_args+=(-Dgst-plugins-good:qt6=disabled)
        fi

        meson setup "$build_dir" \
            --prefix="$prefix" \
            --buildtype="$BUILD_TYPE" \
            --wrap-mode=forcefallback \
            --strip \
            "${cross_args[@]}" \
            -Dauto_features=disabled \
            -Dgst-full-libraries=video,gl \
            -Dgpl=enabled \
            -Dlibav=enabled \
            -Dorc=enabled \
            "${qt_args[@]}" \
            -Dbase=enabled \
            -Dgst-plugins-base:app=enabled \
            -Dgst-plugins-base:gl=enabled \
            -Dgst-plugins-base:gl_api=opengl,gles2 \
            -Dgst-plugins-base:gl_platform=cgl,eagl \
            -Dgst-plugins-base:gl_winsys=cocoa \
            -Dgst-plugins-base:playback=enabled \
            -Dgst-plugins-base:tcp=enabled \
            -Dgood=enabled \
            -Dgst-plugins-good:isomp4=enabled \
            -Dgst-plugins-good:matroska=enabled \
            -Dgst-plugins-good:osxaudio=enabled \
            -Dgst-plugins-good:rtp=enabled \
            -Dgst-plugins-good:rtpmanager=enabled \
            -Dgst-plugins-good:rtsp=enabled \
            -Dgst-plugins-good:udp=enabled \
            -Dbad=enabled \
            -Dgst-plugins-bad:applemedia=enabled \
            -Dgst-plugins-bad:gl=enabled \
            -Dgst-plugins-bad:mpegtsdemux=enabled \
            -Dgst-plugins-bad:rtp=enabled \
            -Dgst-plugins-bad:sdp=enabled \
            -Dgst-plugins-bad:videoparsers=enabled \
            -Dgst-plugins-bad:x265=enabled \
            -Dugly=enabled \
            -Dgst-plugins-ugly:x264=enabled
    fi

    # Build
    log_info "Compiling GStreamer for $target_arch..."
    meson compile -C "$build_dir" -j "$NUM_JOBS"

    # Install
    log_info "Installing GStreamer for $target_arch..."
    meson install -C "$build_dir"
}

# Build for requested architecture(s)
if [[ "$ARCH" == "universal" ]]; then
    build_arch "x86_64"
    build_arch "arm64"

    # Create universal binaries using lipo
    log_info "Creating universal binaries..."
    mkdir -p "$INSTALL_PREFIX"

    # Copy structure from arm64
    cp -R "$INSTALL_PREFIX-arm64/"* "$INSTALL_PREFIX/"

    # Find and lipo all Mach-O files
    find "$INSTALL_PREFIX-arm64" -type f \( -name "*.dylib" -o -name "*.a" -o -perm +111 \) | while read -r arm64_file; do
        rel_path="${arm64_file#$INSTALL_PREFIX-arm64/}"
        x86_file="$INSTALL_PREFIX-x86_64/$rel_path"
        out_file="$INSTALL_PREFIX/$rel_path"

        if [[ -f "$x86_file" ]]; then
            # Check if it's a Mach-O file
            if file "$arm64_file" | grep -q "Mach-O"; then
                mkdir -p "$(dirname "$out_file")"
                lipo -create "$arm64_file" "$x86_file" -output "$out_file" 2>/dev/null || \
                    cp "$arm64_file" "$out_file"
            fi
        fi
    done

    log_ok "Universal binaries created"
else
    build_arch "$ARCH"
fi

# Success
echo ""
log_ok "GStreamer $GST_VERSION installed successfully!"
echo ""
echo "To use with QGroundControl, set:"
echo "  export PKG_CONFIG_PATH=\"$INSTALL_PREFIX/lib/pkgconfig:\$PKG_CONFIG_PATH\""
echo "  export GST_PLUGIN_PATH=\"$INSTALL_PREFIX/lib/gstreamer-1.0\""
echo "  export DYLD_LIBRARY_PATH=\"$INSTALL_PREFIX/lib:\$DYLD_LIBRARY_PATH\""
echo ""

# Output for CI
# Archive naming: gstreamer-1.0-macos-{arch}-{version}.tar.xz
ARCHIVE_NAME="gstreamer-1.0-macos-$ARCH-$GST_VERSION"
if [[ -n "${GITHUB_OUTPUT:-}" ]]; then
    echo "gstreamer_prefix=$INSTALL_PREFIX" >> "$GITHUB_OUTPUT"
    echo "gstreamer_version=$GST_VERSION" >> "$GITHUB_OUTPUT"
    echo "gstreamer_arch=$ARCH" >> "$GITHUB_OUTPUT"
    echo "archive_name=$ARCHIVE_NAME" >> "$GITHUB_OUTPUT"
fi

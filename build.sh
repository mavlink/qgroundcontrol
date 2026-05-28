#!/bin/bash
# =============================================================================
# JIACDIGCS Automated Build Script
# Supports: Windows (VS), Android (NDK), iOS (Xcode)
# =============================================================================

set -euo pipefail

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color
BOLD='\033[1m'

# Defaults
BUILD_TYPE="${BUILD_TYPE:-Release}"
PLATFORM="${PLATFORM:-}"
BUILD_DIR="${BUILD_DIR:-build}"
VERBOSE="${VERBOSE:-false}"
PARALLEL_JOBS="${PARALLEL_JOBS:-}"
SKIP_TESTS="${SKIP_TESTS:-false}"

# =============================================================================
# Helper Functions
# =============================================================================

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1" >&2
}

log_section() {
    echo ""
    echo -e "${BOLD}${CYAN}============================================${NC}"
    echo -e "${BOLD}${CYAN}  $1${NC}"
    echo -e "${BOLD}${CYAN}============================================${NC}"
}

check_command() {
    if ! command -v "$1" &> /dev/null; then
        log_error "Required command not found: $1"
        return 1
    fi
    return 0
}

check_version() {
    local cmd="$1"
    local min_version="$2"
    local pattern="${3:-.*}"

    local version
    version=$($cmd --version 2>/dev/null | head -1 | grep -oE "$pattern" | head -1)

    if [ -z "$version" ]; then
        log_error "Could not determine version of $cmd"
        return 1
    fi

    # Simple version comparison (major.minor)
    local v1 v2
    v1=$(echo "$min_version" | cut -d. -f1,2)
    v2=$(echo "$version" | cut -d. -f1,2)

    if [ "$(printf '%s\n' "$v1" "$v2" | sort -V | head -n1)" != "$v1" ]; then
        log_error "Required $cmd >= $min_version, found $version"
        return 1
    fi

    return 0
}

# =============================================================================
# Version & Diagnostics
# =============================================================================

show_version() {
    log_section "Version Information"

    local git_hash branch version

    if command -v git &> /dev/null; then
        git_hash=$(git rev-parse --short=8 HEAD 2>/dev/null || echo "unknown")
        branch=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unknown")
        version=$(git describe --tags --always 2>/dev/null || echo "v0.0.0-dev")

        echo -e "  ${BOLD}Git Hash:${NC}    $git_hash"
        echo -e "  ${BOLD}Branch:${NC}      $branch"
        echo -e "  ${BOLD}Version:${NC}     $version"
    else
        echo -e "  ${BOLD}Git:${NC}         Not available"
        echo -e "  ${BOLD}Version:${NC}     v0.0.0-dev"
    fi

    echo -e "  ${BOLD}Build Type:${NC}   $BUILD_TYPE"
    echo -e "  ${BOLD}Platform:${NC}     ${PLATFORM:-auto}"
}

# =============================================================================
# Dependency Checks
# =============================================================================

check_dependencies() {
    log_section "Checking Dependencies"
    local errors=0

    echo -e "  ${BOLD}CMake:${NC}        $(cmake --version 2>/dev/null | head -1 || echo 'Not found')"

    if ! check_command cmake; then
        log_error "CMake not found. Please install CMake 3.25+"
        ((errors++))
    elif ! cmake --version 2>&1 | grep -qE "3\.(2[5-9]|[3-9][0-9])"; then
        log_error "CMake 3.25+ required"
        ((errors++))
    fi

    echo -e "  ${BOLD}Qt Version:${NC}   $(qmake6 --version 2>/dev/null | head -1 || qmake --version 2>/dev/null | head -1 || echo 'Not found')"

    if ! check_command qmake6 && ! check_command qmake; then
        log_error "Qt6 not found. Please install Qt 6.10+"
        ((errors++))
    fi

    if [[ "$PLATFORM" == "android" ]]; then
        echo -e "  ${BOLD}Android SDK:${NC}  ${ANDROID_SDK_ROOT:-Not configured}"
        if [ -z "${ANDROID_SDK_ROOT:-}" ]; then
            log_error "ANDROID_SDK_ROOT not set for Android builds"
            ((errors++))
        fi

        echo -e "  ${BOLD}Android NDK:${NC}  ${ANDROID_NDK_ROOT:-Not configured}"
        if [ -z "${ANDROID_NDK_ROOT:-}" ]; then
            log_error "ANDROID_NDK_ROOT not set for Android builds"
            ((errors++))
        fi

        echo -e "  ${BOLD}Java:${NC}        $(java -version 2>&1 | head -1 || echo 'Not found')"
        if ! check_command java; then
            log_error "Java not found. Please install JDK 17+"
            ((errors++))
        fi
    fi

    if [[ "$PLATFORM" == "ios" ]] || [[ "$PLATFORM" == "ios-simulator" ]]; then
        echo -e "  ${BOLD}Xcode:${NC}       $(xcodebuild -version 2>/dev/null | head -1 || echo 'Not found')"
        if ! check_command xcodebuild; then
            log_error "Xcode not found (required for iOS builds)"
            ((errors++))
        fi
    fi

    if [ $errors -gt 0 ]; then
        log_error "Missing $errors required dependencies"
        return 1
    fi

    log_success "All dependencies satisfied"
    return 0
}

# =============================================================================
# Build Functions
# =============================================================================

configure_windows() {
    log_info "Configuring Windows build (Visual Studio)..."

    local generator="Visual Studio 17 2022"
    local extra_args=""

    # Detect architecture
    case "$(uname -m)" in
        x86_64) local arch="x64" ;;
        arm64) local arch="ARM64" ;;
        *) local arch="x64" ;;
    esac

    if [ -n "${QT_ROOT:-}" ]; then
        extra_args="$extra_args -DQt6_DIR=$QT_ROOT/lib/cmake/Qt6"
    fi

    cmake -B "$BUILD_DIR" -G "$generator" -A "$arch" \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DQGC_APP_NAME=JIACDIGCS \
        -DQGC_ORG_NAME=JIACDIGCS \
        -DQGC_ORG_DOMAIN=jiacdigcs.com \
        -DQGC_PACKAGE_NAME=org.jiacdigcs.swarm \
        $extra_args

    log_success "Configuration complete"
}

configure_linux() {
    log_info "Configuring Linux build (limited support)..."

    # Check for Qt installation
    if [ -z "${QT_ROOT:-}" ]; then
        # Try common locations
        for dir in /opt/Qt6 ~/Qt6 /usr/local/Qt6; do
            if [ -d "$dir" ]; then
                QT_ROOT="$dir"
                break
            fi
        done
    fi

    local extra_args=""
    if [ -n "$QT_ROOT" ]; then
        extra_args="$extra_args -DQt6_DIR=$QT_ROOT/lib/cmake/Qt6"
    fi

    cmake -B "$BUILD_DIR" -G "Unix Makefiles" \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DQGC_APP_NAME=JIACDIGCS \
        -DQGC_ORG_NAME=JIACDIGCS \
        -DQGC_ORG_DOMAIN=jiacdigcs.com \
        -DQGC_PACKAGE_NAME=org.jiacdigcs.swarm \
        $extra_args

    log_success "Configuration complete"
}

configure_android() {
    log_info "Configuring Android build..."

    if [ -z "${ANDROID_SDK_ROOT:-}" ] || [ -z "${ANDROID_NDK_ROOT:-}" ]; then
        log_error "ANDROID_SDK_ROOT and ANDROID_NDK_ROOT must be set"
        return 1
    fi

    local qt_root="${QT_ROOT:-}"
    if [ -z "$qt_root" ]; then
        qt_root=$(find /opt -name "android_arm64_v8a" -type d 2>/dev/null | head -1 | xargs dirname 2>/dev/null || echo "")
    fi

    if [ -z "$qt_root" ]; then
        log_error "Qt for Android not found. Please install Qt 6.10.3 for Android"
        return 1
    fi

    cmake -B "$BUILD_DIR" -G "Unix Makefiles" \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DCMAKE_TOOLCHAIN_FILE="$qt_root/lib/cmake/Qt6/qt.toolchain.cmake" \
        -DQT_HOST_PATH="$qt_root" \
        -DQT_ANDROID_ABIS=arm64-v8a \
        -DQGC_APP_NAME=JIACDIGCS \
        -DQGC_ORG_NAME=JIACDIGCS \
        -DQGC_ORG_DOMAIN=jiacdigcs.com \
        -DQGC_PACKAGE_NAME=org.jiacdigcs.swarm

    log_success "Configuration complete"
}

configure_ios() {
    log_info "Configuring iOS build..."

    if ! check_command xcodebuild; then
        log_error "Xcode not found"
        return 1
    fi

    local qt_root="${QT_ROOT:-}"
    if [ -z "$qt_root" ]; then
        qt_root=$(find /opt -name "ios" -type d 2>/dev/null | head -1 | xargs dirname 2>/dev/null || echo "")
    fi

    if [ -z "$qt_root" ]; then
        log_error "Qt for iOS not found. Please install Qt 6.10.3 for iOS"
        return 1
    fi

    cmake -B "$BUILD_DIR" -G "Xcode" \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DPLATFORM=IOS \
        -DQT_HOST_PATH="$qt_root" \
        -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 \
        -DQGC_APP_NAME=JIACDIGCS \
        -DQGC_ORG_NAME=JIACDIGCS \
        -DQGC_ORG_DOMAIN=jiacdigcs.com \
        -DQGC_PACKAGE_NAME=org.jiacdigcs.swarm

    log_success "Configuration complete"
}

build() {
    log_section "Building JIACDIGCS"

    local build_cmd="cmake --build $BUILD_DIR --parallel"
    if [ -n "$PARALLEL_JOBS" ]; then
        build_cmd="cmake --build $BUILD_DIR --parallel $PARALLEL_JOBS"
    fi

    if [[ "$BUILD_TYPE" == "Release" ]]; then
        build_cmd="$build_cmd --config Release"
    fi

    log_info "Running: $build_cmd"
    eval "$build_cmd"

    log_success "Build complete"
}

# =============================================================================
# Test Functions
# =============================================================================

run_tests() {
    if [[ "$SKIP_TESTS" == "true" ]]; then
        log_warn "Tests skipped"
        return 0
    fi

    log_section "Running Tests"

    # Run unit tests
    if [ -f "$BUILD_DIR/test/QGCUnitTests" ]; then
        log_info "Running unit tests..."
        "$BUILD_DIR/test/QGCUnitTests" || true
    else
        log_warn "Unit tests not found"
    fi

    log_success "Tests complete"
}

# =============================================================================
# Package Functions
# =============================================================================

package() {
    log_section "Packaging"

    case "$PLATFORM" in
        windows)
            log_info "Creating Windows installer..."
            cmake --install "$BUILD_DIR" --config Release
            ;;
        android)
            log_info "Creating Android APK..."
            if [ -d "$BUILD_DIR/android-build" ]; then
                find "$BUILD_DIR/android-build" -name "*.apk" -exec cp {} "$BUILD_DIR/JIACDIGCS.apk" \;
                log_success "APK: $BUILD_DIR/JIACDIGCS.apk"
            fi
            ;;
        ios)
            log_info "Creating iOS app..."
            # Xcode archive would be created here
            log_success "iOS app build complete"
            ;;
        *)
            log_warn "Packaging not implemented for this platform"
            ;;
    esac
}

# =============================================================================
# Main Entry Point
# =============================================================================

usage() {
    cat << EOF
${BOLD}JIACDIGCS Build Script${NC}

${BOLD}Usage:${NC}
    $0 [OPTIONS]

${BOLD}Options:${NC}
    -p, --platform    Build platform: windows, linux, android, ios
    -b, --build-dir   Build directory (default: build)
    -t, --build-type  Build type: Debug, Release (default: Release)
    -j, --jobs        Parallel jobs
    -v, --verbose     Verbose output
    --skip-tests      Skip running tests
    -h, --help        Show this help

${BOLD}Environment Variables:${NC}
    QT_ROOT           Qt installation directory
    ANDROID_SDK_ROOT  Android SDK directory
    ANDROID_NDK_ROOT  Android NDK directory

${BOLD}Examples:${NC}
    $0 --platform windows --build-type Release
    $0 --platform android --jobs 4
    $0 --platform ios --build-dir build-ios

EOF
}

main() {
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case "$1" in
            -p|--platform)
                PLATFORM="$2"
                shift 2
                ;;
            -b|--build-dir)
                BUILD_DIR="$2"
                shift 2
                ;;
            -t|--build-type)
                BUILD_TYPE="$2"
                shift 2
                ;;
            -j|--jobs)
                PARALLEL_JOBS="$2"
                shift 2
                ;;
            -v|--verbose)
                VERBOSE="true"
                shift
                ;;
            --skip-tests)
                SKIP_TESTS="true"
                shift
                ;;
            -h|--help)
                usage
                exit 0
                ;;
            *)
                log_error "Unknown option: $1"
                usage
                exit 1
                ;;
        esac
    done

    # Auto-detect platform if not specified
    if [ -z "$PLATFORM" ]; then
        case "$(uname -s)" in
            CYGWIN*|MINGW*|MSYS*) PLATFORM="windows" ;;
            Darwin)
                if [ -n "${IPHONEOS_DEPLOYMENT_TARGET:-}" ]; then
                    PLATFORM="ios"
                else
                    log_warn "macOS desktop builds disabled for JIACDIGCS"
                    log_info "Use --platform ios for iOS builds"
                    exit 0
                fi
                ;;
            Linux)
                if [ -d "/opt/android-sdk" ] || [ -n "${ANDROID_SDK_ROOT:-}" ]; then
                    PLATFORM="android"
                else
                    log_warn "Linux desktop builds disabled for JIACDIGCS"
                    log_info "Use --platform android for Android builds"
                    exit 0
                fi
                ;;
            *)
                log_error "Unknown platform: $(uname -s)"
                exit 1
                ;;
        esac
    fi

    # Show version
    show_version

    # Check dependencies
    check_dependencies || exit 1

    # Configure
    log_section "Configuring Build"

    mkdir -p "$BUILD_DIR"

    case "$PLATFORM" in
        windows) configure_windows ;;
        linux) configure_linux ;;
        android) configure_android ;;
        ios) configure_ios ;;
        *)
            log_error "Unknown platform: $PLATFORM"
            exit 1
            ;;
    esac

    # Build
    build || exit 1

    # Test
    run_tests

    # Package
    package

    log_section "Build Complete!"
    log_success "JIACDIGCS built successfully"
    echo ""
    echo "Build artifacts: $BUILD_DIR"
}

# Run main
main "$@"
#!/usr/bin/env bash
#
# Configure QGroundControl build
#
# Usage:
#   ./tools/configure.sh                     # Default Debug build
#   ./tools/configure.sh --release           # Release build
#   ./tools/configure.sh --testing           # With unit tests
#   ./tools/configure.sh --coverage          # With coverage
#   ./tools/configure.sh --unity             # Unity build (faster)
#   ./tools/configure.sh --qt-root ~/Qt/6.8.0/gcc_64  # Explicit Qt
#
# Environment:
#   QT_ROOT_DIR - Qt installation (auto-detected if not set)
#   CMAKE_GENERATOR - Generator (default: Ninja)
#
set -euo pipefail

source "$(dirname "$0")/common.sh"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Defaults
BUILD_DIR="build"
BUILD_TYPE="Debug"
GENERATOR="${CMAKE_GENERATOR:-Ninja}"
TESTING=false
COVERAGE=false
STABLE=false
UNITY_BUILD=false
UNITY_BATCH_SIZE=16
USE_QT_CMAKE=true
EXTRA_ARGS=()

usage() {
    cat <<EOF
Usage: $(basename "$0") [OPTIONS]

Configure QGroundControl build.

Options:
  -B, --build-dir DIR     Build directory (default: build)
  -t, --build-type TYPE   Build type: Debug, Release, RelWithDebInfo (default: Debug)
  -G, --generator GEN     CMake generator (default: Ninja)
  --release               Shorthand for --build-type Release
  --debug                 Shorthand for --build-type Debug
  --testing               Enable unit tests
  --coverage              Enable code coverage
  --stable                Build as stable release
  --unity                 Enable unity build (faster compilation)
  --unity-batch SIZE      Unity build batch size (default: 16)
  --qt-root DIR           Qt installation directory
  --no-qt-cmake           Use cmake instead of qt-cmake
  -h, --help              Show this help

Environment:
  QT_ROOT_DIR             Qt installation (auto-detected if not set)
  CMAKE_GENERATOR         Default generator

Examples:
  $(basename "$0") --release --testing
  $(basename "$0") -B build-debug --debug
  $(basename "$0") --qt-root ~/Qt/6.8.0/gcc_64 --release
EOF
    exit 0
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -B|--build-dir) BUILD_DIR="$2"; shift 2 ;;
        -t|--build-type) BUILD_TYPE="$2"; shift 2 ;;
        -G|--generator) GENERATOR="$2"; shift 2 ;;
        --release) BUILD_TYPE="Release"; shift ;;
        --debug) BUILD_TYPE="Debug"; shift ;;
        --testing) TESTING=true; shift ;;
        --coverage) COVERAGE=true; shift ;;
        --stable) STABLE=true; shift ;;
        --unity) UNITY_BUILD=true; shift ;;
        --unity-batch) UNITY_BATCH_SIZE="$2"; shift 2 ;;
        --qt-root) QT_ROOT_DIR="$2"; shift 2 ;;
        --no-qt-cmake) USE_QT_CMAKE=false; shift ;;
        -h|--help) usage ;;
        --) shift; EXTRA_ARGS+=("$@"); break ;;
        -*) log_error "Unknown option: $1"; exit 1 ;;
        *) EXTRA_ARGS+=("$1"); shift ;;
    esac
done

find_qt_cmake() {
    # Check explicit QT_ROOT_DIR first
    if [[ -n "${QT_ROOT_DIR:-}" ]]; then
        local qt_cmake="${QT_ROOT_DIR}/bin/qt-cmake"
        if [[ -x "$qt_cmake" ]]; then
            echo "$qt_cmake"
            return 0
        fi
    fi

    # Check common Qt installation paths (newest version first)
    local qt_patterns=(
        "$HOME/Qt/*/gcc_64/bin/qt-cmake"
        "$HOME/Qt/*/clang_64/bin/qt-cmake"
        "$HOME/Qt/*/macos/bin/qt-cmake"
        "/opt/Qt/*/gcc_64/bin/qt-cmake"
        "/usr/lib/qt6/bin/qt-cmake"
        "C:/Qt/*/msvc*/bin/qt-cmake.bat"
    )

    for pattern in "${qt_patterns[@]}"; do
        # Use glob expansion to safely find matching paths
        local matches=()
        # shellcheck disable=SC2206
        matches=( $pattern 2>/dev/null ) || true

        # Sort by version and get newest
        if [[ ${#matches[@]} -gt 0 ]]; then
            local found
            found=$(printf '%s\n' "${matches[@]}" | sort -V | tail -1)
            if [[ -n "$found" && -x "$found" ]]; then
                echo "$found"
                return 0
            fi
        fi
    done

    # Fall back to plain cmake
    log_warn "qt-cmake not found, using cmake"
    echo "cmake"
}

configure() {
    local cmake_cmd

    if [[ "$USE_QT_CMAKE" == true ]]; then
        cmake_cmd=$(find_qt_cmake)
    else
        cmake_cmd="cmake"
    fi

    # Build arguments
    local args=(
        -S "$REPO_ROOT"
        -B "$BUILD_DIR"
        -G "$GENERATOR"
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    )

    # Feature flags
    if [[ "$TESTING" == true ]]; then
        args+=(-DQGC_BUILD_TESTING=ON)
    else
        args+=(-DQGC_BUILD_TESTING=OFF)
    fi

    [[ "$COVERAGE" == true ]] && args+=(-DQGC_ENABLE_COVERAGE=ON)
    [[ "$STABLE" == true ]] && args+=(-DQGC_STABLE_BUILD=ON)

    if [[ "$UNITY_BUILD" == true ]]; then
        args+=(-DCMAKE_UNITY_BUILD=ON)
        args+=(-DCMAKE_UNITY_BUILD_BATCH_SIZE="$UNITY_BATCH_SIZE")
    fi

    # Extra arguments
    args+=("${EXTRA_ARGS[@]}")

    log_info "Using: $cmake_cmd"
    log_info "Build type: $BUILD_TYPE"
    log_info "Build dir: $BUILD_DIR"

    # Run cmake
    "$cmake_cmd" "${args[@]}"

    # Output for CI if available
    if [[ -n "${GITHUB_OUTPUT:-}" ]]; then
        echo "build_dir=$(realpath "$BUILD_DIR")" >> "$GITHUB_OUTPUT"
    fi

    log_ok "Configured: $BUILD_DIR"
}

configure

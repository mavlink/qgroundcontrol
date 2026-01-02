#!/usr/bin/env bash
#
# Run QGroundControl unit tests
#
# Usage:
#   ./tools/run-tests.sh                     # Run all tests
#   ./tools/run-tests.sh --filter "Vehicle*" # Filter tests
#   ./tools/run-tests.sh --timeout 600       # Custom timeout
#   ./tools/run-tests.sh --xml               # JUnit XML output
#   ./tools/run-tests.sh --headless          # Force offscreen mode
#
# Environment:
#   QT_QPA_PLATFORM - Qt platform plugin (default: auto-detect)
#
set -euo pipefail

source "$(dirname "$0")/common.sh"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Defaults
BUILD_DIR="build"
BUILD_TYPE=""
BINARY_PATH=""
TEST_FILTER=""
TIMEOUT=300
OUTPUT_XML=false
OUTPUT_FILE=""
HEADLESS=false

usage() {
    cat <<EOF
Usage: $(basename "$0") [OPTIONS]

Run QGroundControl unit tests.

Options:
  -B, --build-dir DIR     Build directory (default: build)
  -t, --build-type TYPE   Build type to find binary in (auto-detect)
  --binary PATH           Explicit path to QGroundControl binary
  --filter PATTERN        Test filter pattern
  --timeout SECS          Timeout in seconds (default: 300)
  --xml                   Generate JUnit XML output
  --output FILE           Output file for XML results
  --headless              Force headless/offscreen mode
  -h, --help              Show this help

Examples:
  $(basename "$0")                           # Run all tests
  $(basename "$0") --filter "MAVLink*"       # Filter tests
  $(basename "$0") -B build-debug --xml      # Debug build with XML output
EOF
    exit 0
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -B|--build-dir) BUILD_DIR="$2"; shift 2 ;;
        -t|--build-type) BUILD_TYPE="$2"; shift 2 ;;
        --binary) BINARY_PATH="$2"; shift 2 ;;
        --filter) TEST_FILTER="$2"; shift 2 ;;
        --timeout) TIMEOUT="$2"; shift 2 ;;
        --xml) OUTPUT_XML=true; shift ;;
        --output) OUTPUT_FILE="$2"; shift 2 ;;
        --headless) HEADLESS=true; shift ;;
        -h|--help) usage ;;
        *) log_error "Unknown option: $1"; exit 1 ;;
    esac
done

detect_platform() {
    case "$(uname -s)" in
        Linux*)  echo "linux" ;;
        Darwin*) echo "macos" ;;
        MINGW*|MSYS*|CYGWIN*) echo "windows" ;;
        *) echo "linux" ;;
    esac
}

find_binary() {
    # Try using find-binary.sh if available
    local find_script="$REPO_ROOT/.github/scripts/find-binary.sh"
    if [[ -x "$find_script" ]]; then
        local args=(--build-dir "$BUILD_DIR")
        [[ -n "$BUILD_TYPE" ]] && args+=(--build-type "$BUILD_TYPE")

        local result
        result=$("$find_script" "${args[@]}" 2>/dev/null | grep "^binary_path=" | cut -d= -f2) || true
        if [[ -n "$result" && -f "$result" ]]; then
            echo "$result"
            return 0
        fi
    fi

    # Fallback: direct search
    local platform
    platform=$(detect_platform)

    local binary_name="QGroundControl"
    [[ "$platform" == "windows" ]] && binary_name="QGroundControl.exe"

    # Check common locations
    local locations=(
        "$BUILD_DIR/$binary_name"
        "$BUILD_DIR/Debug/$binary_name"
        "$BUILD_DIR/Release/$binary_name"
        "$BUILD_DIR/RelWithDebInfo/$binary_name"
    )

    if [[ -n "$BUILD_TYPE" ]]; then
        # Prioritize specified build type
        locations=("$BUILD_DIR/$BUILD_TYPE/$binary_name" "${locations[@]}")
    fi

    for loc in "${locations[@]}"; do
        if [[ -f "$loc" ]]; then
            echo "$loc"
            return 0
        fi
    done

    log_error "Binary not found in $BUILD_DIR"
    return 1
}

run_tests() {
    local binary
    if [[ -n "$BINARY_PATH" ]]; then
        binary="$BINARY_PATH"
    else
        binary=$(find_binary)
    fi

    if [[ ! -f "$binary" ]]; then
        log_error "Binary not found: $binary"
        exit 1
    fi

    chmod +x "$binary" 2>/dev/null || true

    log_info "Binary: $binary"
    log_info "Timeout: ${TIMEOUT}s"

    # Build test arguments
    local test_args=(--unittest)
    if [[ -n "$TEST_FILTER" ]]; then
        test_args=(--unittest:"$TEST_FILTER")
        log_info "Filter: $TEST_FILTER"
    fi

    if [[ "$OUTPUT_XML" == true ]]; then
        OUTPUT_FILE="${OUTPUT_FILE:-$(dirname "$binary")/junit-results.xml}"
        test_args+=(--unittest-output "$OUTPUT_FILE")
        log_info "Output: $OUTPUT_FILE"
    fi

    # Platform detection
    local platform
    platform=$(detect_platform)

    # Set Qt platform for headless if needed
    if [[ "$HEADLESS" == true ]]; then
        export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}"
        log_info "Platform: offscreen (forced)"
    fi

    # Run with display handling
    local exit_code=0
    set +e

    if [[ "$platform" == "linux" && -z "${DISPLAY:-}" && "$HEADLESS" != true ]]; then
        # Linux without display requires xvfb
        if command -v xvfb-run &>/dev/null; then
            log_info "Running with xvfb-run (no DISPLAY)"
            timeout "$TIMEOUT" xvfb-run -a "$binary" "${test_args[@]}"
            exit_code=$?
        else
            log_warn "xvfb-run not found, using offscreen platform"
            export QT_QPA_PLATFORM=offscreen
            timeout "$TIMEOUT" "$binary" "${test_args[@]}"
            exit_code=$?
        fi
    else
        timeout "$TIMEOUT" "$binary" "${test_args[@]}"
        exit_code=$?
    fi
    set -e

    # Output for CI
    if [[ -n "${GITHUB_OUTPUT:-}" ]]; then
        echo "exit_code=$exit_code" >> "$GITHUB_OUTPUT"
        echo "passed=$( [[ $exit_code -eq 0 ]] && echo true || echo false )" >> "$GITHUB_OUTPUT"
        [[ -n "${OUTPUT_FILE:-}" ]] && echo "output_file=$OUTPUT_FILE" >> "$GITHUB_OUTPUT"
    fi

    if [[ $exit_code -eq 0 ]]; then
        log_ok "Tests passed"
    else
        log_error "Tests failed (exit code: $exit_code)"
    fi

    return $exit_code
}

run_tests

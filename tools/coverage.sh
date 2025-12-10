#!/usr/bin/env bash
#
# Generate code coverage reports for QGroundControl
#
# Usage:
#   ./tools/coverage.sh                    # Build with coverage and run tests
#   ./tools/coverage.sh --report           # Generate report only (after tests)
#   ./tools/coverage.sh --open             # Generate and open report in browser
#   ./tools/coverage.sh --clean            # Clean coverage data
#   ./tools/coverage.sh --xml              # Generate XML only (for CI tools)
#
# Requirements:
#   - gcovr (pip install gcovr)
#
# This script wraps CMake coverage targets defined in cmake/modules/Coverage.cmake

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info()  { echo -e "${BLUE}[INFO]${NC} $*"; }
log_ok()    { echo -e "${GREEN}[OK]${NC} $*"; }
log_warn()  { echo -e "${YELLOW}[WARN]${NC} $*"; }
log_error() { echo -e "${RED}[ERROR]${NC} $*" >&2; }

# Defaults
BUILD_DIR="$REPO_ROOT/build-coverage"
REPORT_ONLY=false
OPEN_REPORT=false
CLEAN_ONLY=false
XML_ONLY=false

show_help() {
    head -14 "$0" | tail -12
    exit 0
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            ;;
        -r|--report)
            REPORT_ONLY=true
            shift
            ;;
        -o|--open)
            OPEN_REPORT=true
            shift
            ;;
        -c|--clean)
            CLEAN_ONLY=true
            shift
            ;;
        --xml)
            XML_ONLY=true
            shift
            ;;
        -b|--build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        *)
            log_error "Unknown option: $1"
            exit 1
            ;;
    esac
done

check_dependencies() {
    if ! command -v gcovr &> /dev/null; then
        log_error "gcovr not found"
        log_info "Install with: pip install gcovr"
        exit 1
    fi
}

clean_coverage() {
    log_info "Cleaning coverage data..."
    rm -rf "$BUILD_DIR"
    log_ok "Coverage data cleaned"
}

configure_build() {
    if [[ -f "$BUILD_DIR/CMakeCache.txt" ]]; then
        # Check if already configured with coverage
        if grep -q "QGC_ENABLE_COVERAGE:BOOL=ON" "$BUILD_DIR/CMakeCache.txt" 2>/dev/null; then
            log_info "Using existing coverage build configuration"
            return
        fi
    fi

    log_info "Configuring build with coverage..."
    cmake -B "$BUILD_DIR" -S "$REPO_ROOT" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DQGC_ENABLE_COVERAGE=ON \
        -DQGC_BUILD_TESTING=ON \
        -G Ninja

    log_ok "Build configured"
}

build_project() {
    log_info "Building project..."
    cmake --build "$BUILD_DIR" --parallel
    log_ok "Build complete"
}

run_tests() {
    log_info "Running tests..."
    # Run via ctest for proper test discovery
    ctest --test-dir "$BUILD_DIR" --output-on-failure --timeout 300 || true
    log_ok "Tests complete"
}

generate_report() {
    log_info "Generating coverage report..."

    if [[ "$XML_ONLY" == true ]]; then
        cmake --build "$BUILD_DIR" --target coverage-report
    else
        cmake --build "$BUILD_DIR" --target coverage-report
    fi

    echo ""
    log_ok "Coverage report generated:"
    log_info "  HTML: $BUILD_DIR/coverage.html"
    log_info "  XML:  $BUILD_DIR/coverage.xml"
}

open_report() {
    local report="$BUILD_DIR/coverage.html"
    if [[ ! -f "$report" ]]; then
        log_error "Report not found. Run coverage first."
        exit 1
    fi

    log_info "Opening coverage report..."

    if command -v xdg-open &> /dev/null; then
        xdg-open "$report"
    elif command -v open &> /dev/null; then
        open "$report"
    else
        log_warn "Could not open browser. Report at: $report"
    fi
}

# Main
cd "$REPO_ROOT"
check_dependencies

if [[ "$CLEAN_ONLY" == true ]]; then
    clean_coverage
    exit 0
fi

if [[ "$REPORT_ONLY" == true ]]; then
    generate_report
    if [[ "$OPEN_REPORT" == true ]]; then
        open_report
    fi
    exit 0
fi

# Full coverage workflow
configure_build
build_project
run_tests
generate_report

if [[ "$OPEN_REPORT" == true ]]; then
    open_report
fi

log_ok "Coverage complete!"

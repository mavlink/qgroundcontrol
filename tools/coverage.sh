#!/usr/bin/env bash
#
# Generate code coverage reports for QGroundControl
#
# Usage:
#   ./tools/coverage.sh                    # Build with coverage and run tests
#   ./tools/coverage.sh --report           # Generate HTML report only (after tests)
#   ./tools/coverage.sh --open             # Generate and open report in browser
#   ./tools/coverage.sh --clean            # Clean coverage data
#
# Requirements:
#   - gcov (from GCC) or llvm-cov
#   - lcov and genhtml (for HTML reports)
#   - Optional: gcovr (for Cobertura XML output)
#
# The script configures CMake with coverage flags, runs tests, and generates reports.

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
COVERAGE_DIR="$REPO_ROOT/coverage"
REPORT_ONLY=false
OPEN_REPORT=false
CLEAN_ONLY=false
TEST_FILTER=""

show_help() {
    head -16 "$0" | tail -14
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
        -t|--test)
            TEST_FILTER="$2"
            shift 2
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
    local missing=()

    if ! command -v lcov &> /dev/null; then
        missing+=("lcov")
    fi
    if ! command -v genhtml &> /dev/null; then
        missing+=("genhtml (part of lcov)")
    fi

    if [[ ${#missing[@]} -gt 0 ]]; then
        log_error "Missing dependencies: ${missing[*]}"
        log_info "Install with: sudo apt install lcov"
        exit 1
    fi
}

clean_coverage() {
    log_info "Cleaning coverage data..."
    rm -rf "$BUILD_DIR"
    rm -rf "$COVERAGE_DIR"
    find "$REPO_ROOT" -name "*.gcda" -delete 2>/dev/null || true
    find "$REPO_ROOT" -name "*.gcno" -delete 2>/dev/null || true
    log_ok "Coverage data cleaned"
}

configure_build() {
    log_info "Configuring build with coverage..."

    cmake -B "$BUILD_DIR" -S "$REPO_ROOT" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_C_FLAGS="--coverage -fprofile-arcs -ftest-coverage" \
        -DCMAKE_CXX_FLAGS="--coverage -fprofile-arcs -ftest-coverage" \
        -DCMAKE_EXE_LINKER_FLAGS="--coverage" \
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

    local test_args="--unittest"
    if [[ -n "$TEST_FILTER" ]]; then
        test_args="--unittest:$TEST_FILTER"
    fi

    # Run tests (continue even if some fail)
    # shellcheck disable=SC2086  # Intentional word splitting for test args
    "$BUILD_DIR/QGroundControl" $test_args || true

    log_ok "Tests complete"
}

generate_report() {
    log_info "Generating coverage report..."

    mkdir -p "$COVERAGE_DIR"

    # Capture coverage data
    lcov --capture \
        --directory "$BUILD_DIR" \
        --output-file "$COVERAGE_DIR/coverage.info" \
        --ignore-errors mismatch \
        --rc lcov_branch_coverage=1

    # Remove system headers and test files from coverage
    lcov --remove "$COVERAGE_DIR/coverage.info" \
        '/usr/*' \
        '*/build/*' \
        '*/test/*' \
        '*/libs/*' \
        '*/_deps/*' \
        '*/Qt/*' \
        --output-file "$COVERAGE_DIR/coverage.filtered.info" \
        --rc lcov_branch_coverage=1

    # Generate HTML report
    genhtml "$COVERAGE_DIR/coverage.filtered.info" \
        --output-directory "$COVERAGE_DIR/html" \
        --title "QGroundControl Coverage Report" \
        --legend \
        --branch-coverage \
        --highlight

    # Print summary
    echo ""
    log_ok "Coverage report generated: $COVERAGE_DIR/html/index.html"

    # Show summary statistics
    lcov --summary "$COVERAGE_DIR/coverage.filtered.info" 2>&1 | grep -E "(lines|functions|branches)"
}

open_report() {
    local report="$COVERAGE_DIR/html/index.html"
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

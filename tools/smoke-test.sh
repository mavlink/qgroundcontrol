#!/usr/bin/env bash
#
# Smoke test for QGroundControl tools
#
# Verifies all tools work end-to-end:
# - Script sourcing (common.sh)
# - Help texts for all major scripts
# - Python tool imports
# - Config file reading
# - Directory structure
#
# Usage:
#   ./tools/smoke-test.sh              # Run all tests
#   ./tools/smoke-test.sh --verbose    # Show detailed output
#   ./tools/smoke-test.sh --help       # Show this help

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Source common utilities
source "$SCRIPT_DIR/common.sh"

# Configuration
VERBOSE=false
PASSED=0
FAILED=0
FAILURES=()

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -h|--help)
            head -15 "$0" | tail -11
            exit 0
            ;;
        *)
            log_error "Unknown option: $1"
            exit 1
            ;;
    esac
done

# =============================================================================
# Test Helper Functions
# =============================================================================

run_test() {
    local test_name="$1"
    local test_cmd="$2"

    if [[ "$VERBOSE" == true ]]; then
        log_info "Running: $test_name"
    fi

    if eval "$test_cmd" &> /dev/null; then
        log_ok "✓ $test_name"
        ((PASSED++))
        return 0
    else
        log_error "✗ $test_name"
        FAILURES+=("$test_name")
        ((FAILED++))
        return 1
    fi
}

# =============================================================================
# Test Categories
# =============================================================================

print_header "Script Sourcing Tests"

run_test "Source common.sh without errors" \
    "bash -c \"source '$SCRIPT_DIR/common.sh'; type log_info > /dev/null 2>&1\"" || true

# =============================================================================

print_header "Help Text Tests"

run_test "configure.sh --help exits with 0" \
    "timeout 5 '$SCRIPT_DIR/configure.sh' --help" || true

run_test "run-tests.sh --help exits with 0" \
    "timeout 5 '$SCRIPT_DIR/run-tests.sh' --help" || true

run_test "analyze.sh --help exits with 0" \
    "timeout 5 '$SCRIPT_DIR/analyze.sh' --help" || true

run_test "coverage.sh --help exits with 0" \
    "timeout 5 '$SCRIPT_DIR/coverage.sh' --help" || true

run_test "clean.sh --help exits with 0" \
    "timeout 5 '$SCRIPT_DIR/clean.sh' --help" || true

run_test "check-deps.sh --help exits with 0" \
    "timeout 5 '$SCRIPT_DIR/check-deps.sh' --help" || true

run_test "pre-commit.sh --help exits with 0" \
    "timeout 5 '$SCRIPT_DIR/pre-commit.sh' --help" || true

# =============================================================================

print_header "Python Tool Tests"

run_test "install-qt.py --help succeeds" \
    "timeout 5 python3 '$SCRIPT_DIR/setup/install-qt.py' --help" || true

run_test "read-config.py --help succeeds" \
    "timeout 5 python3 '$SCRIPT_DIR/setup/read-config.py' --help" || true

run_test "build-gstreamer.py --help succeeds" \
    "timeout 5 python3 '$SCRIPT_DIR/setup/gstreamer/build-gstreamer.py' --help" || true

run_test "qgc_locator.py can be imported" \
    "python3 -c \"import sys; sys.path.insert(0, '$SCRIPT_DIR'); from locators import qgc_locator\"" || true

run_test "qgc-lupdate-json.py --help succeeds" \
    "timeout 5 python3 '$SCRIPT_DIR/translations/qgc-lupdate-json.py' --help" || true

# =============================================================================

print_header "Config Reading Tests"

run_test "build-config.json exists and is valid JSON" \
    "python3 -c \"import json; json.load(open('$REPO_ROOT/.github/build-config.json'))\"" || true

run_test "build-config.json contains qt_version" \
    "python3 -c \"import json; config = json.load(open('$REPO_ROOT/.github/build-config.json')); assert 'qt_version' in config\"" || true

# Extract the qt_version for display
qt_version=""
if command -v jq &> /dev/null; then
    qt_version=$(jq -r '.qt_version' "$REPO_ROOT/.github/build-config.json" 2>/dev/null || echo "")
fi
if [[ -z "$qt_version" ]]; then
    # Fallback to Python if jq is not available
    qt_version=$(python3 -c "import json; print(json.load(open('$REPO_ROOT/.github/build-config.json')).get('qt_version', ''))" 2>/dev/null || echo "")
fi
if [[ -n "$qt_version" ]]; then
    log_ok "  Qt version from config: $qt_version"
fi

# =============================================================================

print_header "Directory Structure Tests"

run_test "Directory exists: setup/" \
    "test -d '$SCRIPT_DIR/setup'" || true

run_test "Directory exists: debuggers/" \
    "test -d '$SCRIPT_DIR/debuggers'" || true

run_test "Directory exists: translations/" \
    "test -d '$SCRIPT_DIR/translations'" || true

run_test "Directory exists: simulation/" \
    "test -d '$SCRIPT_DIR/simulation'" || true

run_test "Directory exists: analyzers/" \
    "test -d '$SCRIPT_DIR/analyzers'" || true

run_test "Directory exists: generators/" \
    "test -d '$SCRIPT_DIR/generators'" || true

run_test "Directory exists: locators/" \
    "test -d '$SCRIPT_DIR/locators'" || true

run_test "Directory exists: common/" \
    "test -d '$SCRIPT_DIR/common'" || true

run_test "Directory exists: tests/" \
    "test -d '$SCRIPT_DIR/tests'" || true

run_test "Directory exists: configs/" \
    "test -d '$SCRIPT_DIR/configs'" || true

run_test "Directory exists: schemas/" \
    "test -d '$SCRIPT_DIR/schemas'" || true

# =============================================================================

print_header "Script Syntax Tests"

run_test "configure.sh is valid shell script" \
    "bash -n '$SCRIPT_DIR/configure.sh'" || true

run_test "run-tests.sh is valid shell script" \
    "bash -n '$SCRIPT_DIR/run-tests.sh'" || true

run_test "analyze.sh is valid shell script" \
    "bash -n '$SCRIPT_DIR/analyze.sh'" || true

run_test "common.sh is valid shell script" \
    "bash -n '$SCRIPT_DIR/common.sh'" || true

# =============================================================================

print_header "Summary"

total=$((PASSED + FAILED))
echo ""
echo "Tests passed: ${GREEN}$PASSED${NC}/$total"

if [[ $FAILED -gt 0 ]]; then
    echo "Tests failed: ${RED}$FAILED${NC}/$total"
    echo ""
    log_error "Failed tests:"
    for failure in "${FAILURES[@]}"; do
        echo "  - $failure"
    done
    echo ""
    exit 1
else
    echo "Tests failed: ${GREEN}0${NC}/$total"
    echo ""
    log_ok "All smoke tests passed!"
    exit 0
fi

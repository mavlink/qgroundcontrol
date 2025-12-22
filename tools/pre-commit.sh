#!/usr/bin/env bash
#
# Run pre-commit checks with formatted output
#
# Usage:
#   ./tools/pre-commit.sh              # Run on all files
#   ./tools/pre-commit.sh --changed    # Run only on changed files (vs master)
#   ./tools/pre-commit.sh --install    # Install pre-commit hooks
#   ./tools/pre-commit.sh --update     # Update hook versions
#   ./tools/pre-commit.sh --ci         # CI mode (machine-readable output)
#
# Environment:
#   PRE_COMMIT_OUTPUT  - File to write output (default: stdout)
#   GITHUB_STEP_SUMMARY - GitHub Actions step summary file
#

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
MODE="all"
CI_MODE=false
OUTPUT_FILE=""

show_help() {
    head -13 "$0" | tail -11
    exit 0
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            ;;
        -c|--changed)
            MODE="changed"
            shift
            ;;
        -i|--install)
            MODE="install"
            shift
            ;;
        -u|--update)
            MODE="update"
            shift
            ;;
        --ci)
            CI_MODE=true
            shift
            ;;
        -o|--output)
            OUTPUT_FILE="$2"
            shift 2
            ;;
        *)
            log_error "Unknown option: $1"
            show_help
            ;;
    esac
done

cd "$REPO_ROOT"

# Check for pre-commit
if ! command -v pre-commit &> /dev/null; then
    log_error "pre-commit not found"
    log_info "Install with: pip install pre-commit"
    log_info "Or run: ./tools/pre-commit.sh --install"
    exit 1
fi

# Handle install/update modes
if [[ "$MODE" == "install" ]]; then
    log_info "Installing pre-commit and hooks..."
    pip install pre-commit
    pre-commit install
    pre-commit install --hook-type commit-msg
    log_ok "Pre-commit hooks installed"
    exit 0
fi

if [[ "$MODE" == "update" ]]; then
    log_info "Updating pre-commit hooks..."
    pre-commit autoupdate
    log_ok "Hooks updated. Review changes in .pre-commit-config.yaml"
    exit 0
fi

# Build pre-commit arguments
ARGS=(
    "--show-diff-on-failure"
    "--color=always"
)

if [[ "$MODE" == "changed" ]]; then
    # Check if we can compare against master
    if git rev-parse --verify master &>/dev/null || git rev-parse --verify origin/master &>/dev/null; then
        log_info "Running on files changed vs master..."
        ARGS+=("--from-ref" "master" "--to-ref" "HEAD")
    else
        log_warn "master branch not available, running on all files"
        ARGS+=("--all-files")
    fi
else
    ARGS+=("--all-files")
fi

# Create temp file for output capture
TEMP_OUTPUT=$(mktemp)
trap 'rm -f "$TEMP_OUTPUT"' EXIT

log_info "Running pre-commit checks..."
echo ""

# Run pre-commit and capture output
set +e
pre-commit run "${ARGS[@]}" 2>&1 | tee "$TEMP_OUTPUT"
EXIT_CODE=${PIPESTATUS[0]}
set -e

echo ""

# Parse results - ensure numeric values
PASSED=$(grep -c 'Passed' "$TEMP_OUTPUT" 2>/dev/null || true)
FAILED=$(grep -c 'Failed' "$TEMP_OUTPUT" 2>/dev/null || true)
SKIPPED=$(grep -c 'Skipped' "$TEMP_OUTPUT" 2>/dev/null || true)
# Default to 0 if empty or non-numeric
[[ "$PASSED" =~ ^[0-9]+$ ]] || PASSED=0
[[ "$FAILED" =~ ^[0-9]+$ ]] || FAILED=0
[[ "$SKIPPED" =~ ^[0-9]+$ ]] || SKIPPED=0

# Summary
if [[ $EXIT_CODE -eq 0 ]]; then
    log_ok "All checks passed ($PASSED passed)"
else
    log_error "Some checks failed ($PASSED passed, $FAILED failed)"
fi

# Write to output file if specified
if [[ -n "$OUTPUT_FILE" ]] || [[ -n "${PRE_COMMIT_OUTPUT:-}" ]]; then
    OUTPUT_FILE="${OUTPUT_FILE:-$PRE_COMMIT_OUTPUT}"
    cp "$TEMP_OUTPUT" "$OUTPUT_FILE"
    log_info "Output written to: $OUTPUT_FILE"
fi

# CI mode: write structured output
if [[ "$CI_MODE" == true ]]; then
    # Write to GITHUB_OUTPUT if available
    if [[ -n "${GITHUB_OUTPUT:-}" ]]; then
        {
            echo "exit_code=$EXIT_CODE"
            echo "passed=$PASSED"
            echo "failed=$FAILED"
            echo "skipped=$SKIPPED"
        } >> "$GITHUB_OUTPUT"

        # Write summary to GITHUB_OUTPUT (multiline with unique delimiter)
        DELIMITER="PRECOMMIT_SUMMARY_$(date +%s)"
        {
            echo "summary<<${DELIMITER}"
            grep -E '\.\.\.\.\.\.\.\.\.\.' "$TEMP_OUTPUT" 2>/dev/null | head -40 | sed 's/\x1b\[[0-9;]*m//g' || echo "No results"
            echo "${DELIMITER}"
        } >> "$GITHUB_OUTPUT"
    fi

    # Write to GITHUB_STEP_SUMMARY if available
    if [[ -n "${GITHUB_STEP_SUMMARY:-}" ]]; then
        {
            echo "## Pre-commit Results"
            echo ""
            if [[ $EXIT_CODE -eq 0 ]]; then
                echo "✅ **All checks passed**"
            else
                echo "⚠️ **Some checks failed**"
            fi
            echo ""
            echo "| Status | Count |"
            echo "|--------|-------|"
            echo "| ✅ Passed | $PASSED |"
            echo "| ❌ Failed | $FAILED |"
            if [[ $SKIPPED -gt 0 ]]; then
                echo "| ⏭️ Skipped | $SKIPPED |"
            fi
            echo ""
            echo "<details>"
            echo "<summary>Hook Results</summary>"
            echo ""
            echo '```'
            grep -E '\.\.\.\.\.\.\.\.\.\.' "$TEMP_OUTPUT" 2>/dev/null | head -40 | sed 's/\x1b\[[0-9;]*m//g' || echo "No results"
            echo '```'
            echo "</details>"

            # Show modified files if any
            if ! git diff --quiet 2>/dev/null; then
                echo ""
                echo "<details>"
                echo "<summary>Files Modified by Hooks</summary>"
                echo ""
                echo '```'
                git diff --stat 2>/dev/null || true
                echo '```'
                echo "</details>"
            fi
        } >> "$GITHUB_STEP_SUMMARY"
    fi
fi

# Show fix instructions on failure
if [[ $EXIT_CODE -ne 0 ]]; then
    echo ""
    log_info "To fix issues locally:"
    echo "  1. Run: pre-commit run --all-files"
    echo "  2. Review and stage changes: git add -u"
    echo "  3. Amend your commit: git commit --amend --no-edit"
fi

exit $EXIT_CODE

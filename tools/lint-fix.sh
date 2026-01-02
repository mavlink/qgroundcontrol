#!/usr/bin/env bash
#
# Auto-apply pre-commit fixes for formatting and style issues
#
# Usage:
#   ./tools/lint-fix.sh              # Fix all files
#   ./tools/lint-fix.sh --changed    # Fix only changed files vs master
#   ./tools/lint-fix.sh --stage      # Fix and stage changes
#   ./tools/lint-fix.sh --check      # Check only (don't fix)
#   ./tools/lint-fix.sh -h           # Show help
#
# Fixable Hooks:
#   - clang-format (C++ formatting)
#   - ruff-format (Python formatting)
#   - prettier (JSON/YAML/Markdown via pre-commit-hooks)
#   - cmake-format (CMake files)
#   - trailing-whitespace
#   - end-of-file-fixer
#
# Environment:
#   LOG_LEVEL - Control verbosity (0=silent, 1=error, 2=warn, 3=info, 4=debug)
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Source shared utilities
source "$SCRIPT_DIR/common.sh"

# Find repo root
REPO_ROOT=$(find_repo_root "$SCRIPT_DIR")

# Defaults
MODE="all"
STAGE_CHANGES=false
CHECK_MODE=false

show_help() {
    cat << 'EOF'
Auto-apply pre-commit fixes for formatting and style issues

Usage:
  ./tools/lint-fix.sh              # Fix all files
  ./tools/lint-fix.sh --changed    # Fix only changed files vs master
  ./tools/lint-fix.sh --stage      # Fix and stage changes
  ./tools/lint-fix.sh --check      # Check only (don't fix)
  ./tools/lint-fix.sh -h           # Show help

Options:
  --all                Run on all files (default)
  --changed            Run only on changed files vs master branch
  --stage              Stage fixed files after running
  --check              Check only, don't fix (same as pre-commit.sh)
  -h, --help           Show this help message

Fixable Hooks:
  - clang-format       C++ code formatting
  - ruff-format        Python code formatting
  - pretty-format-json JSON formatting
  - cmake-format       CMake file formatting
  - trailing-whitespace Trailing whitespace cleanup
  - end-of-file-fixer  EOF enforcement

Examples:
  # Fix all Python and JSON formatting issues
  ./tools/lint-fix.sh

  # Fix only files changed in this branch
  ./tools/lint-fix.sh --changed

  # Fix and immediately stage changes
  ./tools/lint-fix.sh --stage

  # Just check what would be fixed (don't modify files)
  ./tools/lint-fix.sh --check

EOF
    exit 0
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            ;;
        -a|--all)
            MODE="all"
            shift
            ;;
        -c|--changed)
            MODE="changed"
            shift
            ;;
        -s|--stage)
            STAGE_CHANGES=true
            shift
            ;;
        -C|--check)
            CHECK_MODE=true
            shift
            ;;
        *)
            log_error "Unknown option: $1"
            echo ""
            show_help
            ;;
    esac
done

cd "$REPO_ROOT"

# Check for pre-commit
if ! command -v pre-commit &> /dev/null; then
    log_error "pre-commit not found"
    log_info "Install with: pip install pre-commit"
    exit 1
fi

# Determine which hooks to run (fixable only)
# These are hooks that modify files in-place
FIXABLE_HOOKS=(
    "clang-format"
    "ruff-format"
    "pretty-format-json"
    "cmake-format"
    "trailing-whitespace"
    "end-of-file-fixer"
)

# Determine scope
SCOPE_ARGS=()
if [[ "$MODE" == "changed" ]]; then
    DEFAULT_BRANCH=$(get_default_branch)
    if git rev-parse --verify "$DEFAULT_BRANCH" &>/dev/null 2>&1 || git rev-parse --verify "origin/$DEFAULT_BRANCH" &>/dev/null 2>&1; then
        log_info "Running on files changed vs $DEFAULT_BRANCH..."
        SCOPE_ARGS=("--from-ref" "$DEFAULT_BRANCH" "--to-ref" "HEAD")
    else
        log_warn "$DEFAULT_BRANCH not available, running on all files"
        SCOPE_ARGS=("--all-files")
    fi
else
    SCOPE_ARGS=("--all-files")
fi

# Check mode: run without modifications (just check)
if [[ "$CHECK_MODE" == true ]]; then
    log_info "Running in check mode (no modifications)..."
    cd "$REPO_ROOT"
    pre-commit run --show-diff-on-failure --color=always "${SCOPE_ARGS[@]}"
    exit $?
fi

# Create temp file for capturing state
TEMP_OUTPUT=$(mktemp)
trap 'rm -f "$TEMP_OUTPUT"' EXIT

log_info "Running auto-fix formatters on $(if [[ "$MODE" == "changed" ]]; then echo "changed files"; else echo "all files"; fi)..."
echo ""

# Capture state before
MODIFIED_BEFORE=$(git ls-files -m 2>/dev/null | wc -l)

# Run fixable hooks with direct formatter invocation
# This gives us more control over auto-fix options than pre-commit alone

# Helper function to run formatters safely
run_formatter() {
    local name="$1"
    local cmd="$2"

    if command -v "$cmd" &>/dev/null || [[ -x "$cmd" ]]; then
        log_info "Running $name..."
        eval "$cmd" 2>&1 | tee -a "$TEMP_OUTPUT" || log_warn "$name exited with error"
    else
        log_debug "$name not found (skipping)"
    fi
}

# Execute formatters in order
run_formatter "ruff-format" "ruff format --quiet tools/ test/ 2>/dev/null || true"
run_formatter "clang-format" "find '$REPO_ROOT' -type f \\( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' -o -name '*.c' \\) -not -path '*/build/*' -not -path '*/libs/*' -exec clang-format -i {} \\; 2>/dev/null || true"
run_formatter "cmake-format" "find '$REPO_ROOT' -type f \\( -name 'CMakeLists.txt' -o -name '*.cmake' \\) -not -path '*/build/*' -not -path '*/libs/*' -exec cmake-format -i {} \\; 2>/dev/null || true"

# Run trailing-whitespace and end-of-file-fixer through pre-commit for consistency
log_info "Running trailing-whitespace and end-of-file-fixer..."
cd "$REPO_ROOT"
pre-commit run trailing-whitespace end-of-file-fixer --color=always "${SCOPE_ARGS[@]}" 2>&1 | tee -a "$TEMP_OUTPUT" || true

echo ""

# Capture state after
MODIFIED_AFTER=$(git ls-files -m 2>/dev/null | wc -l)

# Check if there are any modifications
if ! git diff --quiet 2>/dev/null; then
    log_info "Changes made to:"
    echo ""
    git diff --stat
    echo ""

    # Show preview of diff (first 50 lines)
    DIFF_LINES=$(git diff | wc -l)
    if [[ $DIFF_LINES -gt 0 ]]; then
        log_info "Diff preview (first 50 lines):"
        echo ""
        git diff --color=always | head -50
        if [[ $DIFF_LINES -gt 50 ]]; then
            echo ""
            echo "... (diff truncated, run 'git diff' to see all $DIFF_LINES lines)"
        fi
    fi

    echo ""
else
    log_ok "No changes needed - all files are properly formatted!"
    exit 0
fi

# Stage changes if requested
if [[ "$STAGE_CHANGES" == true ]]; then
    log_info "Staging modified files..."
    git add -u
    echo ""
    log_ok "Changes staged for commit"
    echo ""
    log_info "Next steps:"
    echo "  1. Review staged changes: git diff --cached"
    echo "  2. Commit: git commit -m 'style: auto-apply formatting fixes'"
else
    echo ""
    log_info "To review and commit these changes:"
    echo "  1. Review changes: git diff"
    echo "  2. Stage all changes: git add -u"
    echo "  3. Commit: git commit -m 'style: auto-apply formatting fixes'"
    echo ""
    log_info "Or run with --stage flag to auto-stage changes:"
    echo "  ./tools/lint-fix.sh --stage"
fi

log_ok "Formatting complete!"
exit 0

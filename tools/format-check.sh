#!/usr/bin/env bash
#
# Check or apply clang-format to source files
#
# Usage:
#   ./tools/format-check.sh                # Format changed files (vs master)
#   ./tools/format-check.sh --check        # Check only, don't modify (for CI)
#   ./tools/format-check.sh --all          # Format all source files
#   ./tools/format-check.sh src/Vehicle/   # Format specific directory
#
# Requires: clang-format (version 17+ recommended)

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
CHECK_ONLY=false
FORMAT_ALL=false
TARGET_PATH=""

show_help() {
    head -12 "$0" | tail -10
    exit 0
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            ;;
        -c|--check)
            CHECK_ONLY=true
            shift
            ;;
        -a|--all)
            FORMAT_ALL=true
            shift
            ;;
        *)
            TARGET_PATH="$1"
            shift
            ;;
    esac
done

# Check for clang-format
if ! command -v clang-format &> /dev/null; then
    log_error "clang-format not found"
    log_info "Install with: sudo apt install clang-format"
    exit 1
fi

# Extract major version (works on both GNU and BSD grep)
CLANG_FORMAT_VERSION=$(clang-format --version | grep -Eo '[0-9]+' | head -1)
log_info "Using clang-format version $CLANG_FORMAT_VERSION"

# Get files to format
get_files() {
    if [[ -n "$TARGET_PATH" ]]; then
        find "$REPO_ROOT/$TARGET_PATH" -type f \( -name "*.cc" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) 2>/dev/null
    elif [[ "$FORMAT_ALL" == true ]]; then
        find "$REPO_ROOT/src" "$REPO_ROOT/test" -type f \( -name "*.cc" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) 2>/dev/null
    else
        # Only changed files vs master
        git -C "$REPO_ROOT" diff --name-only master... -- '*.cc' '*.cpp' '*.h' '*.hpp' 2>/dev/null | \
            xargs -I{} echo "$REPO_ROOT/{}" | \
            while read -r f; do [[ -f "$f" ]] && echo "$f"; done
    fi
}

cd "$REPO_ROOT"

files=$(get_files)

if [[ -z "$files" ]]; then
    log_info "No files to format"
    exit 0
fi

file_count=$(echo "$files" | wc -l)
log_info "Processing $file_count files..."

if [[ "$CHECK_ONLY" == true ]]; then
    # Check mode - verify formatting without modifying
    needs_format=()

    while read -r file; do
        if ! clang-format --dry-run --Werror "$file" 2>/dev/null; then
            needs_format+=("${file#"$REPO_ROOT"/}")
        fi
    done <<< "$files"

    if [[ ${#needs_format[@]} -gt 0 ]]; then
        log_error "The following files need formatting:"
        for f in "${needs_format[@]}"; do
            echo "  $f"
        done
        echo ""
        log_info "Run: ./tools/format-check.sh"
        exit 1
    fi

    log_ok "All files properly formatted"
else
    # Format mode - modify files in place
    formatted=0

    while read -r file; do
        if clang-format -i "$file"; then
            ((formatted++))
        fi
    done <<< "$files"

    log_ok "Formatted $formatted files"

    # Show what changed
    if git -C "$REPO_ROOT" diff --quiet; then
        log_info "No formatting changes needed"
    else
        log_info "Files modified:"
        git -C "$REPO_ROOT" diff --name-only
    fi
fi

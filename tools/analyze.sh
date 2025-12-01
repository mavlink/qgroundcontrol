#!/usr/bin/env bash
#
# Run static analysis on QGroundControl source code
#
# Usage:
#   ./tools/analyze.sh                    # Analyze changed files (vs master)
#   ./tools/analyze.sh --all              # Analyze all source files
#   ./tools/analyze.sh src/Vehicle/       # Analyze specific directory
#   ./tools/analyze.sh --tool clang-tidy  # Use specific tool
#
# Tools:
#   clang-tidy  - Clang static analyzer (default, requires compile_commands.json)
#   cppcheck    - Cppcheck static analyzer

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
TOOL="clang-tidy"
ANALYZE_ALL=false
TARGET_PATH=""
BUILD_DIR="$REPO_ROOT/build"
COMPILE_COMMANDS="$BUILD_DIR/compile_commands.json"

show_help() {
    head -15 "$0" | tail -13
    exit 0
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            ;;
        -a|--all)
            ANALYZE_ALL=true
            shift
            ;;
        -t|--tool)
            TOOL="$2"
            shift 2
            ;;
        -b|--build-dir)
            BUILD_DIR="$2"
            COMPILE_COMMANDS="$BUILD_DIR/compile_commands.json"
            shift 2
            ;;
        *)
            TARGET_PATH="$1"
            shift
            ;;
    esac
done

# Get files to analyze
get_files() {
    if [[ -n "$TARGET_PATH" ]]; then
        find "$REPO_ROOT/$TARGET_PATH" -type f \( -name "*.cc" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) 2>/dev/null
    elif [[ "$ANALYZE_ALL" == true ]]; then
        find "$REPO_ROOT/src" -type f \( -name "*.cc" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \)
    else
        # Only changed files vs master
        git -C "$REPO_ROOT" diff --name-only master... -- '*.cc' '*.cpp' '*.h' '*.hpp' 2>/dev/null | \
            xargs -I{} echo "$REPO_ROOT/{}" | \
            while read -r f; do [[ -f "$f" ]] && echo "$f"; done
    fi
}

run_clang_tidy() {
    if [[ ! -f "$COMPILE_COMMANDS" ]]; then
        log_error "compile_commands.json not found at $COMPILE_COMMANDS"
        log_info "Run: cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
        exit 1
    fi

    if ! command -v clang-tidy &> /dev/null; then
        log_error "clang-tidy not found. Install with: sudo apt install clang-tidy"
        exit 1
    fi

    local files
    files=$(get_files)

    if [[ -z "$files" ]]; then
        log_info "No files to analyze"
        exit 0
    fi

    local file_count
    file_count=$(echo "$files" | wc -l)
    log_info "Running clang-tidy on $file_count files..."

    local exit_code=0
    echo "$files" | while read -r file; do
        echo -n "  Analyzing: ${file#"$REPO_ROOT"/}... "
        if clang-tidy -p "$BUILD_DIR" "$file" 2>/dev/null; then
            echo -e "${GREEN}OK${NC}"
        else
            echo -e "${RED}ISSUES${NC}"
            exit_code=1
        fi
    done

    return $exit_code
}

run_cppcheck() {
    if ! command -v cppcheck &> /dev/null; then
        log_error "cppcheck not found. Install with: sudo apt install cppcheck"
        exit 1
    fi

    local files
    files=$(get_files)

    if [[ -z "$files" ]]; then
        log_info "No files to analyze"
        exit 0
    fi

    local file_count
    file_count=$(echo "$files" | wc -l)
    log_info "Running cppcheck on $file_count files..."

    # Create file list
    local filelist
    filelist=$(mktemp)
    echo "$files" > "$filelist"

    cppcheck \
        --enable=warning,style,performance,portability \
        --std=c++20 \
        --suppress=missingIncludeSystem \
        --suppress=unmatchedSuppression \
        --inline-suppr \
        --file-list="$filelist" \
        --error-exitcode=1 \
        2>&1

    local exit_code=$?
    rm -f "$filelist"
    return $exit_code
}

# Main
cd "$REPO_ROOT"

case "$TOOL" in
    clang-tidy)
        run_clang_tidy
        ;;
    cppcheck)
        run_cppcheck
        ;;
    *)
        log_error "Unknown tool: $TOOL"
        log_info "Available tools: clang-tidy, cppcheck"
        exit 1
        ;;
esac

log_ok "Static analysis complete"

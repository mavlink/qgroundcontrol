#!/usr/bin/env bash
#
# Run code quality tools on QGroundControl source code
#
# Usage:
#   ./tools/analyze.sh                    # Analyze changed files (vs master)
#   ./tools/analyze.sh --all              # Analyze all source files
#   ./tools/analyze.sh src/Vehicle/       # Analyze specific directory
#   ./tools/analyze.sh --tool clang-tidy  # Use specific tool
#   ./tools/analyze.sh --tool clang-format --fix  # Apply formatting
#
# Tools:
#   clang-format - Code formatting (use --fix to apply changes)
#   clang-tidy   - Clang static analyzer (requires compile_commands.json)
#   cppcheck     - Cppcheck static analyzer
#   clazy        - Qt-specific static analyzer (requires compile_commands.json)
#   qmllint      - QML file linter

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
FIX_MODE=false
TARGET_PATH=""
BUILD_DIR="$REPO_ROOT/build"
COMPILE_COMMANDS="$BUILD_DIR/compile_commands.json"

show_help() {
    head -17 "$0" | tail -15
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
        -f|--fix)
            FIX_MODE=true
            shift
            ;;
        -b|--build-dir)
            BUILD_DIR="$2"
            COMPILE_COMMANDS="$BUILD_DIR/compile_commands.json"
            shift 2
            ;;
        *)
            TARGET_PATH="$1"
            # Validate path - prevent traversal and ensure relative
            if [[ "$TARGET_PATH" =~ \.\. ]] || [[ "$TARGET_PATH" = /* ]]; then
                log_error "Invalid path: $TARGET_PATH (must be relative, no '..')"
                exit 1
            fi
            shift
            ;;
    esac
done

# Get C++ files to analyze
get_cpp_files() {
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

# Get QML files to analyze
get_qml_files() {
    if [[ -n "$TARGET_PATH" ]]; then
        find "$REPO_ROOT/$TARGET_PATH" -type f -name "*.qml" 2>/dev/null
    elif [[ "$ANALYZE_ALL" == true ]]; then
        find "$REPO_ROOT/src" -type f -name "*.qml"
    else
        # Only changed files vs master
        git -C "$REPO_ROOT" diff --name-only master... -- '*.qml' 2>/dev/null | \
            xargs -I{} echo "$REPO_ROOT/{}" | \
            while read -r f; do [[ -f "$f" ]] && echo "$f"; done
    fi
}

run_clang_format() {
    if ! command -v clang-format &> /dev/null; then
        log_error "clang-format not found. Install with: sudo apt install clang-format"
        exit 1
    fi

    local version
    version=$(clang-format --version | grep -Eo '[0-9]+' | head -1)
    log_info "Using clang-format version $version"

    local files
    files=$(get_cpp_files)

    if [[ -z "$files" ]]; then
        log_info "No files to check"
        exit 0
    fi

    local file_count
    file_count=$(echo "$files" | wc -l)

    if [[ "$FIX_MODE" == true ]]; then
        log_info "Formatting $file_count files..."
        local formatted=0

        while read -r file; do
            if clang-format -i "$file"; then
                ((formatted++))
            fi
        done <<< "$files"

        log_ok "Formatted $formatted files"

        if git -C "$REPO_ROOT" diff --quiet; then
            log_info "No formatting changes needed"
        else
            log_info "Files modified:"
            git -C "$REPO_ROOT" diff --name-only
        fi
    else
        log_info "Checking formatting on $file_count files..."
        local needs_format=()

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
            log_info "Run: ./tools/analyze.sh --tool clang-format --fix"
            exit 1
        fi

        log_ok "All files properly formatted"
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
    files=$(get_cpp_files)

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
    files=$(get_cpp_files)

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

run_clazy() {
    if [[ ! -f "$COMPILE_COMMANDS" ]]; then
        log_warn "compile_commands.json not found - skipping clazy"
        log_info "Run: cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
        exit 0
    fi

    if ! command -v clazy-standalone &> /dev/null; then
        log_warn "clazy-standalone not found - skipping"
        log_info "Install with: sudo apt install clazy"
        exit 0
    fi

    local files
    files=$(get_cpp_files)

    if [[ -z "$files" ]]; then
        log_info "No files to analyze"
        exit 0
    fi

    local file_count
    file_count=$(echo "$files" | wc -l)
    log_info "Running clazy on $file_count files..."

    # Clazy check levels:
    #   level0: No false positives
    #   level1: Very few false positives
    #   level2: More checks, some false positives
    # Using level1 + specific Qt checks
    local checks="level1,connect-non-signal,lambda-in-connect,overridden-signal"

    local issues_found=0
    local output_file
    output_file=$(mktemp)

    echo "$files" | while read -r file; do
        local relpath="${file#"$REPO_ROOT"/}"
        if clazy-standalone -p "$BUILD_DIR" --checks="$checks" "$file" 2>"$output_file"; then
            : # No issues
        else
            if [[ -s "$output_file" ]]; then
                log_warn "Issues in $relpath:"
                cat "$output_file"
                issues_found=1
            fi
        fi
    done

    rm -f "$output_file"

    if [[ $issues_found -eq 1 ]]; then
        log_warn "Clazy found Qt-specific issues"
        return 1
    fi

    return 0
}

run_qmllint() {
    if ! command -v qmllint &> /dev/null; then
        log_warn "qmllint not found - skipping"
        log_info "Install with: Qt SDK or 'sudo apt install qt6-declarative-dev-tools'"
        exit 0
    fi

    local files
    files=$(get_qml_files)

    if [[ -z "$files" ]]; then
        log_info "No QML files to lint"
        exit 0
    fi

    local file_count
    file_count=$(echo "$files" | wc -l)
    log_info "Running qmllint on $file_count files..."

    local issues_found=0

    while read -r file; do
        if ! qmllint "$file" 2>&1; then
            issues_found=1
        fi
    done <<< "$files"

    if [[ $issues_found -eq 1 ]]; then
        log_warn "QML lint issues found"
        return 1
    fi

    return 0
}

# Main
cd "$REPO_ROOT"

case "$TOOL" in
    clang-format)
        run_clang_format
        ;;
    clang-tidy)
        run_clang_tidy
        ;;
    cppcheck)
        run_cppcheck
        ;;
    clazy)
        run_clazy
        ;;
    qmllint)
        run_qmllint
        ;;
    *)
        log_error "Unknown tool: $TOOL"
        log_info "Available tools: clang-format, clang-tidy, cppcheck, clazy, qmllint"
        exit 1
        ;;
esac

log_ok "Analysis complete"

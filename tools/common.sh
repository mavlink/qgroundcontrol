#!/usr/bin/env bash
#
# Common shell functions for QGroundControl tools
#
# Usage:
#   source "$(dirname "${BASH_SOURCE[0]}")/common.sh"
#   # or
#   SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
#   source "$SCRIPT_DIR/common.sh"
#
# Provides:
#   - Color definitions (RED, GREEN, YELLOW, BLUE, NC)
#   - Logging functions (log_info, log_ok, log_warn, log_error)
#   - find_repo_root() - Find repository root directory
#   - require_command() - Check for required command

# Prevent multiple inclusion
if [[ -n "${_QGC_COMMON_SH_LOADED:-}" ]]; then
    return 0
fi
_QGC_COMMON_SH_LOADED=1

# =============================================================================
# Colors (disabled if not a terminal or NO_COLOR is set)
# =============================================================================

if [[ -t 1 ]] && [[ -z "${NO_COLOR:-}" ]]; then
    RED='\033[0;31m'
    GREEN='\033[0;32m'
    YELLOW='\033[1;33m'
    BLUE='\033[0;34m'
    BOLD='\033[1m'
    NC='\033[0m'  # No Color
else
    RED=''
    GREEN=''
    YELLOW=''
    BLUE=''
    BOLD=''
    NC=''
fi

# =============================================================================
# Logging Functions
# =============================================================================

log_info() {
    echo -e "${BLUE}[INFO]${NC} $*"
}

log_ok() {
    echo -e "${GREEN}[OK]${NC} $*"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $*"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $*" >&2
}

log_debug() {
    if [[ -n "${DEBUG:-}" ]]; then
        echo -e "${BLUE}[DEBUG]${NC} $*" >&2
    fi
}

# =============================================================================
# Utility Functions
# =============================================================================

# Find the repository root (directory containing .git)
# Usage: REPO_ROOT=$(find_repo_root)
find_repo_root() {
    local dir="${1:-$PWD}"
    while [[ "$dir" != "/" ]]; do
        if [[ -d "$dir/.git" ]]; then
            echo "$dir"
            return 0
        fi
        dir=$(dirname "$dir")
    done
    return 1
}

# Check if a command exists, exit with error if not
# Usage: require_command cmake "Install with: sudo apt install cmake"
require_command() {
    local cmd="$1"
    local install_hint="${2:-}"

    if ! command -v "$cmd" &> /dev/null; then
        log_error "$cmd not found"
        if [[ -n "$install_hint" ]]; then
            log_info "$install_hint"
        fi
        exit 1
    fi
}

# Check if a command exists (without exiting)
# Usage: if has_command cmake; then ...
has_command() {
    command -v "$1" &> /dev/null
}

# Print a section header
# Usage: print_header "Building project"
print_header() {
    echo ""
    echo -e "${BOLD}=== $* ===${NC}"
    echo ""
}

# Run a command and show what's being run
# Usage: run_cmd cmake --build build
run_cmd() {
    log_info "Running: $*"
    "$@"
}

# Get script directory (call from your script, not from here)
# Usage: SCRIPT_DIR=$(get_script_dir "${BASH_SOURCE[0]}")
get_script_dir() {
    local source="$1"
    cd "$(dirname "$source")" && pwd
}

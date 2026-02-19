#!/usr/bin/env bash
#
# Check for outdated dependencies and submodules
#
# Usage:
#   ./tools/check-deps.sh              # Check all dependencies
#   ./tools/check-deps.sh --submodules # Check only git submodules
#   ./tools/check-deps.sh --qt         # Check Qt version
#   ./tools/check-deps.sh --update     # Update submodules to latest
#
# Checks:
#   - Git submodules vs upstream
#   - Qt version vs latest
#   - GStreamer version vs latest
#   - Python dependencies

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
CHECK_ALL=true
CHECK_SUBMODULES=false
CHECK_QT=false
UPDATE_DEPS=false

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
        --submodules)
            CHECK_ALL=false
            CHECK_SUBMODULES=true
            shift
            ;;
        --qt)
            CHECK_ALL=false
            CHECK_QT=true
            shift
            ;;
        --update)
            UPDATE_DEPS=true
            shift
            ;;
        *)
            log_error "Unknown option: $1"
            exit 1
            ;;
    esac
done

check_submodules() {
    log_info "Checking git submodules..."

    cd "$REPO_ROOT"

    # Initialize submodules if needed
    git submodule update --init --recursive 2>/dev/null || true

    local outdated=0

    while IFS= read -r line; do
        if [[ -z "$line" ]]; then
            continue
        fi

        # Parse submodule status (status and hash reserved for future use)
        local _status="${line:0:1}"
        local _hash="${line:1:40}"
        local path
        path=$(echo "$line" | awk '{print $2}')

        if [[ ! -d "$REPO_ROOT/$path" ]]; then
            continue
        fi

        cd "$REPO_ROOT/$path"

        # Get current and remote info (reserved for future verbose output)
        local _current_hash _branch
        _current_hash=$(git rev-parse HEAD 2>/dev/null || echo "unknown")
        _branch=$(git symbolic-ref --short HEAD 2>/dev/null || echo "detached")

        # Fetch updates quietly
        git fetch --quiet 2>/dev/null || true

        # Check if behind
        local behind=0
        # shellcheck disable=SC1083  # @{u} is valid git syntax for upstream
        if git rev-parse '@{u}' &>/dev/null; then
            behind=$(git rev-list --count 'HEAD..@{u}' 2>/dev/null || echo "0")
        fi

        if [[ "$behind" -gt 0 ]]; then
            log_warn "$path: $behind commits behind upstream"
            ((outdated++)) || true
        else
            echo -e "  ${GREEN}✓${NC} $path (up to date)"
        fi

        cd "$REPO_ROOT"
    done < <(git submodule status --recursive 2>/dev/null)

    if [[ "$outdated" -eq 0 ]]; then
        log_ok "All submodules up to date"
    else
        log_warn "$outdated submodule(s) have updates available"
        if [[ "$UPDATE_DEPS" == true ]]; then
            log_info "Updating submodules..."
            git submodule update --remote --merge
            log_ok "Submodules updated"
        else
            log_info "Run with --update to update submodules"
        fi
    fi
}

check_qt_version() {
    log_info "Checking Qt version..."

    # Read current version from config
    local config_file="$REPO_ROOT/.github/build-config.json"
    if [[ ! -f "$config_file" ]]; then
        log_warn "build-config.json not found"
        return
    fi

    local current_version
    current_version=$(python3 -c "import json; print(json.load(open('$config_file')).get('qt_version', 'unknown'))")

    echo "  Current: Qt $current_version"

    # Check latest Qt version (from qt.io)
    local latest_info
    if command -v curl &> /dev/null; then
        # Try to get latest version info
        # Extract Qt 6.x versions (portable grep)
        latest_info=$(curl -s "https://download.qt.io/official_releases/qt/" 2>/dev/null | \
            grep -Eo '6\.[0-9]+' | sort -V | tail -1 || echo "")

        if [[ -n "$latest_info" ]]; then
            echo "  Latest minor: Qt $latest_info.x"

            local current_minor="${current_version%.*}"
            if [[ "$current_minor" != "$latest_info" ]]; then
                log_warn "Newer Qt minor version available: $latest_info"
            else
                log_ok "Using latest Qt minor version"
            fi
        fi
    fi

    # Check installed Qt
    if command -v qmake &> /dev/null; then
        local installed
        installed=$(qmake --version 2>/dev/null | grep -Eo '[0-9]+\.[0-9]+\.[0-9]+' || echo "not found")
        echo "  Installed: Qt $installed"
    elif [[ -n "${QT_ROOT_DIR:-}" ]]; then
        echo "  QT_ROOT_DIR: $QT_ROOT_DIR"
    fi
}

check_gstreamer_version() {
    log_info "Checking GStreamer version..."

    # Read current version from config
    local config_file="$REPO_ROOT/.github/build-config.json"
    if [[ ! -f "$config_file" ]]; then
        return
    fi

    local current_version
    current_version=$(python3 -c "import json; print(json.load(open('$config_file')).get('gstreamer_default_version', 'unknown'))")

    echo "  Configured: GStreamer $current_version"

    # Check installed version
    if command -v gst-launch-1.0 &> /dev/null; then
        local installed
        installed=$(gst-launch-1.0 --version 2>/dev/null | head -1 | grep -Eo '[0-9]+\.[0-9]+\.[0-9]+' || echo "not found")
        echo "  Installed: GStreamer $installed"
    fi
}

check_python_deps() {
    log_info "Checking Python dependencies..."

    local req_files=("requirements.txt" "docs/requirements.txt")

    for req in "${req_files[@]}"; do
        if [[ -f "$REPO_ROOT/$req" ]]; then
            echo "  Checking $req..."
            if command -v pip &> /dev/null; then
                # Check for outdated packages
                local outdated
                outdated=$(pip list --outdated --format=columns 2>/dev/null | tail -n +3 || echo "")
                if [[ -n "$outdated" ]]; then
                    echo "$outdated" | while read -r line; do
                        echo "    $line"
                    done
                fi
            fi
        fi
    done
}

check_build_tools() {
    log_info "Checking build tools..."

    local tools=("cmake" "ninja" "ccache" "clang-format" "clang-tidy")

    for tool in "${tools[@]}"; do
        if command -v "$tool" &> /dev/null; then
            local version
            version=$("$tool" --version 2>/dev/null | head -1 || echo "unknown")
            echo -e "  ${GREEN}✓${NC} $tool: $version"
        else
            echo -e "  ${YELLOW}✗${NC} $tool: not installed"
        fi
    done
}

# Main
cd "$REPO_ROOT"

if [[ "$CHECK_ALL" == true ]]; then
    check_submodules
    echo ""
    check_qt_version
    echo ""
    check_gstreamer_version
    echo ""
    check_build_tools
elif [[ "$CHECK_SUBMODULES" == true ]]; then
    check_submodules
elif [[ "$CHECK_QT" == true ]]; then
    check_qt_version
fi

echo ""
log_ok "Dependency check complete"

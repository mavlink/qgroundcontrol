#!/usr/bin/env bash
#
# Clean build artifacts and caches
#
# Usage:
#   ./tools/clean.sh              # Clean build directory
#   ./tools/clean.sh --all        # Clean everything (build, caches, generated files)
#   ./tools/clean.sh --cache      # Clean only caches (ccache, pip, etc.)
#
# This script removes:
#   - build/           CMake build directory
#   - .cache/          Local caches (CPM, clangd index)
#   - .ccache/         ccache storage
#   - *.user           Qt Creator user files
#   - CMakeUserPresets.json

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
log_error() { echo -e "${RED}[ERROR]${NC} $*"; }

# Defaults
CLEAN_ALL=false
CLEAN_CACHE_ONLY=false
DRY_RUN=false

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
        -a|--all)
            CLEAN_ALL=true
            shift
            ;;
        -c|--cache)
            CLEAN_CACHE_ONLY=true
            shift
            ;;
        -n|--dry-run)
            DRY_RUN=true
            shift
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

cd "$REPO_ROOT"

remove_if_exists() {
    local path="$1"
    local desc="${2:-$path}"

    if [[ -e "$path" ]]; then
        if [[ "$DRY_RUN" == true ]]; then
            log_info "Would remove: $desc"
        else
            log_info "Removing: $desc"
            rm -rf "$path"
        fi
    fi
}

clean_build() {
    remove_if_exists "build" "build directory"
    remove_if_exists "CMakeUserPresets.json" "CMake user presets"

    # Qt Creator user files
    find . -maxdepth 1 -name "*.user" -type f 2>/dev/null | while read -r f; do
        remove_if_exists "$f" "Qt Creator user file: $f"
    done

    # CMake generated files in source
    find . -maxdepth 1 -name "CMakeFiles" -type d 2>/dev/null | while read -r d; do
        remove_if_exists "$d" "CMake files: $d"
    done
}

clean_cache() {
    remove_if_exists ".cache" "local cache directory"
    remove_if_exists ".ccache" "ccache directory"

    # clangd index
    remove_if_exists ".clangd" "clangd index"

    # ccache stats (not the cache itself, just local stats)
    if command -v ccache &> /dev/null; then
        if [[ "$DRY_RUN" == true ]]; then
            log_info "Would clear ccache statistics"
        else
            log_info "Clearing ccache statistics"
            ccache --zero-stats 2>/dev/null || true
        fi
    fi
}

clean_generated() {
    # Compiled Python files
    find . -type d -name "__pycache__" 2>/dev/null | while read -r d; do
        remove_if_exists "$d" "Python cache: $d"
    done

    find . -name "*.pyc" -type f 2>/dev/null | while read -r f; do
        remove_if_exists "$f" "Python bytecode: $f"
    done

    # Generated translation files (keep source .ts files)
    # remove_if_exists "translations/*.qm" "compiled translation files"
}

# Main
if [[ "$DRY_RUN" == true ]]; then
    log_warn "Dry run mode - no files will be removed"
fi

if [[ "$CLEAN_CACHE_ONLY" == true ]]; then
    clean_cache
elif [[ "$CLEAN_ALL" == true ]]; then
    clean_build
    clean_cache
    clean_generated
else
    clean_build
fi

log_ok "Clean complete"

# Show disk space recovered
if [[ "$DRY_RUN" != true ]]; then
    echo ""
    log_info "Disk usage: $(du -sh "$REPO_ROOT" 2>/dev/null | cut -f1)"
fi

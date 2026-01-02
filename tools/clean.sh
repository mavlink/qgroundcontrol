#!/usr/bin/env bash
#
# Clean build artifacts and caches
#
# Usage:
#   ./tools/clean.sh              # Clean build directory
#   ./tools/clean.sh --all        # Clean everything (build, caches, generated files)
#   ./tools/clean.sh --cache      # Clean only caches (ccache, pip, etc.)
#   ./tools/clean.sh --dry-run    # Show what would be deleted without removing
#
# This script removes:
#   - build/           CMake build directory
#   - .cache/          Local caches (ccache, clangd index)
#   - *.user           Qt Creator user files
#   - CMakeUserPresets.json
#
# Options:
#   -h, --help         Show this help message
#   -a, --all          Clean everything (build, caches, generated)
#   -c, --cache        Clean only caches
#   -n, --dry-run      Show what would be deleted without removing

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Source shared utilities
source "$(dirname "${BASH_SOURCE[0]}")/common.sh"

# Defaults
CLEAN_ALL=false
CLEAN_CACHE_ONLY=false
DRY_RUN=false

# Dry-run tracking
DRY_RUN_FILES=()
DRY_RUN_DIRS=()
DRY_RUN_SIZES=()

show_help() {
    head -21 "$0" | tail -19
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
            local size=$(du -sh "$path" 2>/dev/null | cut -f1)
            log_warn "Would delete: $desc ($size)"

            # Track statistics
            DRY_RUN_SIZES+=("$size")
            if [[ -d "$path" ]]; then
                DRY_RUN_DIRS+=("$path")
            else
                DRY_RUN_FILES+=("$path")
            fi
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

    # clangd index
    remove_if_exists ".clangd" "clangd index"

    # ccache stats (not the cache itself, just local stats)
    if command -v ccache &> /dev/null; then
        if [[ "$DRY_RUN" == true ]]; then
            log_warn "Would clear: ccache statistics"
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

print_dry_run_summary() {
    if [[ "$DRY_RUN" != true ]] || [[ ${#DRY_RUN_FILES[@]} -eq 0 && ${#DRY_RUN_DIRS[@]} -eq 0 ]]; then
        return
    fi

    echo ""
    log_ok "Dry run complete. Would delete:"
    log_info "  - ${#DRY_RUN_FILES[@]} files"
    log_info "  - ${#DRY_RUN_DIRS[@]} directories"

    # Calculate approximate total size
    local total_size=0
    for size_str in "${DRY_RUN_SIZES[@]}"; do
        # Convert size strings like "1.2G", "500M", "123K" to a simple display
        # For now, just list them individually (more accurate than trying to sum)
        :
    done

    # Show cumulative disk savings (approximate)
    if [[ ${#DRY_RUN_SIZES[@]} -gt 0 ]]; then
        log_info "  - Sizes: ${DRY_RUN_SIZES[*]}"
    fi

    echo ""
    log_warn "Run without --dry-run to actually delete these items:"
    log_info "  ${BASH_SOURCE[0]} [options]"
}

# Main
if [[ "$DRY_RUN" == true ]]; then
    log_warn "DRY RUN MODE - No files will be removed"
    echo ""
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

if [[ "$DRY_RUN" == true ]]; then
    print_dry_run_summary
else
    log_ok "Clean complete"
    echo ""
    log_info "Disk usage: $(du -sh "$REPO_ROOT" 2>/dev/null | cut -f1)"
fi

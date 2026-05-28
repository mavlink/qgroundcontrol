#!/bin/bash
# =============================================================================
# JIACDIGCS Release Script
# Creates a semantic version release with changelog
# =============================================================================

set -euo pipefail

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'
BOLD='\033[1m'

# =============================================================================
# Helper Functions
# =============================================================================

log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1" >&2; }

log_section() {
    echo ""
    echo -e "${BOLD}${CYAN}============================================${NC}"
    echo -e "${BOLD}${CYAN}  $1${NC}"
    echo -e "${BOLD}${CYAN}============================================${NC}"
}

# =============================================================================
# Version Functions
# =============================================================================

get_current_version() {
    local tag
    tag=$(git describe --tags --abbrev=0 2>/dev/null || echo "v0.0.0")
    echo "$tag"
}

get_previous_version() {
    local tags
    tags=$(git tag --sort=-creatordate | grep -E '^v?[0-9]' | head -5)
    local prev=""
    for tag in $tags; do
        if [ "$tag" != "$(get_current_version)" ]; then
            prev="$tag"
            break
        fi
    done
    echo "$prev"
}

bump_version() {
    local current="$1"
    local bump_type="$2"

    # Strip v prefix
    current=$(echo "$current" | sed 's/^v//')

    # Parse version
    IFS='.' read -ra VER <<< "$current"
    local major="${VER[0]:-0}"
    local minor="${VER[1]:-0}"
    local patch="${VER[2]:-0}"

    case "$bump_type" in
        major)
            major=$((major + 1)); minor=0; patch=0 ;;
        minor)
            minor=$((minor + 1)); patch=0 ;;
        patch)
            patch=$((patch + 1)) ;;
        *)
            log_error "Unknown bump type: $bump_type"
            return 1 ;;
    esac

    echo "v$major.$minor.$patch"
}

# =============================================================================
# Changelog Functions
# =============================================================================

generate_changelog() {
    local from_tag="$1"
    local to_tag="$2"
    local output_file="$3"

    log_info "Generating changelog from $from_tag to $to_tag..."

    local changelog="# JIACDIGCS Release Notes\n\n"
    changelog="${changelog}## $to_tag\n\n"
    changelog="${changelog}**Release Date:** $(date +%Y-%m-%d)\n\n"
    changelog="${changelog}**Product:** Professional Multi-UAV Swarm Command and Control Platform\n\n"

    # Get commits
    local commits
    if [ -n "$from_tag" ]; then
        commits=$(git log --format="%s" "$from_tag..HEAD" 2>/dev/null)
    else
        commits=$(git log --format="%s" -50)
    fi

    # Categorize
    local features="" fixes="" ci="" docs="" other=""

    while IFS= read -r msg; do
        case "$msg" in
            *feat*|*add*|*new*)
                features="${features}- ${msg}\n" ;;
            *fix*|*bug*|*patch*)
                fixes="${fixes}- ${msg}\n" ;;
            *ci*|*workflow*|*action*)
                ci="${ci}- ${msg}\n" ;;
            *doc*|*readme*|*changelog*)
                docs="${docs}- ${msg}\n" ;;
            *)
                other="${other}- ${msg}\n" ;;
        esac
    done <<< "$commits"

    # Build sections
    if [ -n "$features" ]; then
        changelog="${changelog}### ✨ Features\n\n$(printf "$features")\n"
    fi
    if [ -n "$fixes" ]; then
        changelog="${changelog}### 🐛 Bug Fixes\n\n$(printf "$fixes")\n"
    fi
    if [ -n "$ci" ]; then
        changelog="${changelog}### 🔧 CI Improvements\n\n$(printf "$ci")\n"
    fi
    if [ -n "$docs" ]; then
        changelog="${changelog}### 📚 Documentation\n\n$(printf "$docs")\n"
    fi
    if [ -n "$other" ]; then
        changelog="${changelog}### 📝 Other Changes\n\n$(printf "$other")\n"
    fi

    # Add Swarm features section
    changelog="${changelog}\n---\n\n"
    changelog="${changelog}## Swarm Capabilities\n\n"
    changelog="${changelog}JIACDIGCS provides professional multi-UAV swarm operations:\n\n"
    changelog="${changelog}- **Formation Control:** Line, V, Grid, Circle, Custom\n"
    changelog="${changelog}- **Synchronized Commands:** Takeoff, Land, RTL, Emergency Stop\n"
    changelog="${changelog}- **Real-time Telemetry:** Live health monitoring for all members\n"
    changelog="${changelog}- **Multi-Vehicle:** Support for up to 20 simultaneous UAVs\n"
    changelog="${changelog}- **Alert System:** Comprehensive swarm health alerts\n"

    # Write
    echo -e "$changelog" > "$output_file"
    log_success "Changelog written to $output_file"
}

# =============================================================================
# Tag Functions
# =============================================================================

create_tag() {
    local tag="$1"
    local message="${2:-}"

    if [ -z "$message" ]; then
        message="Release $tag - JIACDIGCS Professional Multi-UAV Swarm Command and Control Platform"
    fi

    log_info "Creating tag: $tag"
    git tag -a "$tag" -m "$message"

    log_success "Tag created: $tag"
}

push_tag() {
    local tag="$1"
    log_info "Pushing tag to origin..."
    git push origin "$tag"
    log_success "Tag pushed"
}

# =============================================================================
# Release Functions
# =============================================================================

create_release() {
    local version="$1"
    local changelog_file="$2"

    log_section "Creating Release v$version"

    # Check if release exists
    if gh release view "v$version" &>/dev/null; then
        log_warn "Release v$version already exists"
        return 1
    fi

    # Create release
    local body=""
    if [ -f "$changelog_file" ]; then
        body=$(cat "$changelog_file")
    fi

    log_info "Creating GitHub release..."
    gh release create "v$version" \
        --title "JIACDIGCS v$version" \
        --notes "$body" \
        --draft=false

    log_success "Release v$version created!"
}

# =============================================================================
# Main Entry Point
# =============================================================================

usage() {
    cat << EOF
${BOLD}JIACDIGCS Release Script${NC}

${BOLD}Usage:${NC}
    $0 [OPTIONS]

${BOLD}Options:${NC}
    -t, --type         Version bump type: major, minor, patch (default: patch)
    -v, --version      Specific version (overrides --type)
    -m, --message      Tag message
    --dry-run          Show what would be done
    -h, --help         Show this help

${BOLD}Examples:${NC}
    $0 --type patch          # Bump patch version
    $0 --type minor          # Bump minor version
    $0 --version v1.2.3      # Create specific version
    $0 --dry-run             # Preview without creating

EOF
}

main() {
    local bump_type="patch"
    local specific_version=""
    local message=""
    local dry_run=false

    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case "$1" in
            -t|--type)
                bump_type="$2"; shift 2 ;;
            -v|--version)
                specific_version="$2"; shift 2 ;;
            -m|--message)
                message="$2"; shift 2 ;;
            --dry-run)
                dry_run=true; shift ;;
            -h|--help)
                usage; exit 0 ;;
            *)
                log_error "Unknown option: $1"
                usage; exit 1 ;;
        esac
    done

    log_section "JIACDIGCS Release"

    # Get current version
    local current_version
    current_version=$(get_current_version)
    log_info "Current version: $current_version"

    # Calculate new version
    local new_version
    if [ -n "$specific_version" ]; then
        new_version="$specific_version"
    else
        new_version=$(bump_version "$current_version" "$bump_type")
    fi

    log_info "New version: $new_version"

    if $dry_run; then
        log_warn "DRY RUN - No changes made"
        return 0
    fi

    # Get previous version for changelog
    local prev_version
    prev_version=$(get_previous_version)
    log_info "Previous version: ${prev_version:-none}"

    # Generate changelog
    local changelog_file="/tmp/jiacdigcs-changelog-$(date +%s).md"
    generate_changelog "$prev_version" "$new_version" "$changelog_file"

    # Create tag
    create_tag "$new_version" "$message"

    # Push tag (triggers release workflow)
    push_tag "$new_version"

    # Clean up
    rm -f "$changelog_file"

    log_section "Release Complete!"
    log_success "Created release v$new_version"
    echo ""
    echo "View release: https://github.com/$(git remote get-url origin | sed 's/.*github.com[:/]\(.*\)\.git/\1/' | sed 's/.*://')/releases/tag/$new_version"
}

# Run main
main "$@"
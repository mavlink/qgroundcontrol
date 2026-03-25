#!/usr/bin/env bash
#
# Run semantic-release for automated versioning and changelog
#
# Usage:
#   ./tools/release.sh              # Dry-run (preview what would happen)
#   ./tools/release.sh --run        # Actually create release (CI only)
#   ./tools/release.sh --install    # Install semantic-release dependencies
#
# Requires: Node.js 18+, npm
#
# Environment:
#   GITHUB_TOKEN - Required for --run mode (set automatically in CI)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

source "$REPO_ROOT/tools/common/shell-utils.sh"

DRY_RUN=true
INSTALL_DEPS=false

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
        -r|--run)
            DRY_RUN=false
            shift
            ;;
        -i|--install)
            INSTALL_DEPS=true
            shift
            ;;
        *)
            log_error "Unknown option: $1"
            show_help
            ;;
    esac
done

cd "$REPO_ROOT"

# Check for Node.js
if ! command -v node &> /dev/null; then
    log_error "Node.js not found. Install with: https://nodejs.org/"
    exit 1
fi

NODE_VERSION=$(node --version | grep -Eo '[0-9]+' | head -1)
if [[ "$NODE_VERSION" -lt 18 ]]; then
    log_error "Node.js 18+ required (found: $(node --version))"
    exit 1
fi

# Pin versions for reproducibility and security (update via Dependabot)
SR_VERSION="24.2.5"
SR_PACKAGES="semantic-release@$SR_VERSION @semantic-release/changelog@6.0.3 @semantic-release/git@10.0.1 conventional-changelog-conventionalcommits@8.0.0"

# Install to local node_modules if requested
if [[ "$INSTALL_DEPS" == true ]]; then
    log_info "Installing semantic-release dependencies locally..."
    # shellcheck disable=SC2086
    npm install --save-dev $SR_PACKAGES
    log_ok "Dependencies installed"
    exit 0
fi

# Check for config file
if [[ ! -f ".releaserc.json" ]]; then
    log_error ".releaserc.json not found in repository root"
    exit 1
fi

SR_CMD="npx --yes semantic-release@$SR_VERSION"

if [[ "$DRY_RUN" == true ]]; then
    log_info "Running semantic-release in dry-run mode..."
    log_info "This will show what would happen without making changes"
    echo ""

    $SR_CMD --dry-run

    echo ""
    log_ok "Dry-run complete (no changes made)"
    log_info "Run with --run to create an actual release"
else
    if [[ -z "${GITHUB_TOKEN:-}" ]]; then
        log_error "GITHUB_TOKEN environment variable required for --run mode"
        log_info "This should only be run in CI, not locally"
        exit 1
    fi

    log_info "Running semantic-release..."
    $SR_CMD
    log_ok "Release complete"
fi

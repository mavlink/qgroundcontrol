#!/bin/bash
#
# Setup Python development environment for QGroundControl
#
# Usage:
#   ./tools/setup/install-python.sh           # Install CI tools (pre-commit, meson, ninja)
#   ./tools/setup/install-python.sh dev       # Install development tools
#   ./tools/setup/install-python.sh all       # Install everything
#   ./tools/setup/install-python.sh --help    # Show help
#
# This script uses uv (fast) if available, otherwise falls back to pip.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"

# Default group
GROUP="${1:-ci}"

if [[ "$GROUP" == "--help" || "$GROUP" == "-h" ]]; then
    cat << 'EOF'
Setup Python environment for QGroundControl development.

Usage: ./tools/setup/install-python.sh [GROUP]

Groups:
  ci        Pre-commit hooks, meson, ninja (default)
  qt        Qt installation tools (aqtinstall)
  coverage  Code coverage tools (gcovr)
  dev       Development tools (jinja2, pyyaml, pymavlink)
  lsp       LSP server (pygls, lsprotocol)
  all       Everything

Examples:
  ./tools/setup/install-python.sh              # CI tools only
  ./tools/setup/install-python.sh dev          # Development tools
  ./tools/setup/install-python.sh ci,coverage  # Multiple groups

To install uv (recommended, 10-100x faster than pip):
  curl -LsSf https://astral.sh/uv/install.sh | sh
EOF
    exit 0
fi

echo "Setting up Python environment with group: $GROUP"

cd "$REPO_ROOT"

# Check if uv is available
if command -v uv &> /dev/null; then
    echo "Using uv (fast mode)"

    # Create venv if it doesn't exist
    if [[ ! -d .venv ]]; then
        uv venv .venv
    fi

    # Activate and install
    source .venv/bin/activate
    uv pip install -e "./tools[${GROUP}]"
else
    echo "Using pip (install uv for faster installs: curl -LsSf https://astral.sh/uv/install.sh | sh)"

    # Create venv if it doesn't exist
    if [[ ! -d .venv ]]; then
        python3 -m venv .venv
    fi

    # Activate and install
    source .venv/bin/activate
    pip install --quiet --upgrade pip
    pip install -e "./tools[${GROUP}]"
fi

echo ""
echo "Done! Activate the environment with:"
echo "  source .venv/bin/activate"
echo ""
echo "Installed packages:"
if command -v uv &> /dev/null; then
    uv pip list
else
    pip list
fi

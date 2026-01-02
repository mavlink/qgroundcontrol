#!/usr/bin/env bash
#
# Run pre-commit checks with formatted output
#
# Thin delegation script that calls pre_commit.py for all functionality.
#
# Usage:
#   ./tools/pre-commit.sh              # Run on all files
#   ./tools/pre-commit.sh --changed    # Run only on changed files (vs master)
#   ./tools/pre-commit.sh --install    # Install pre-commit hooks
#   ./tools/pre-commit.sh --update     # Update hook versions
#   ./tools/pre-commit.sh --ci         # CI mode (machine-readable output)
#
# See pre_commit.py for full documentation.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PYTHON_SCRIPT="$SCRIPT_DIR/pre_commit.py"

# Find Python 3
find_python() {
    if command -v python3 &> /dev/null; then
        echo "python3"
    elif command -v python &> /dev/null && python --version 2>&1 | grep -q "Python 3"; then
        echo "python"
    else
        return 1
    fi
}

PYTHON=$(find_python) || { echo "Error: Python 3 is required" >&2; exit 1; }

# Delegate to Python
exec "$PYTHON" "$PYTHON_SCRIPT" "$@"

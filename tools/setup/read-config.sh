#!/usr/bin/env bash
#
# Read build configuration from .github/build-config.json
#
# Thin delegation script that calls read-config.py for all functionality.
# Source this script to export variables, or call with arguments to query config.
#
# Usage:
#   source read-config.sh                    # Export all variables
#   ./read-config.sh                         # Print all config values
#   ./read-config.sh --get qt_version        # Get single value
#   ./read-config.sh --json                  # Output as JSON
#   ./read-config.sh --github-output         # Output for GitHub Actions
#
# See read-config.py for full documentation.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PYTHON_SCRIPT="$SCRIPT_DIR/read-config.py"

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

# For sourcing: evaluate bash export statements from Python
if [[ "${1:-}" != "--get" && "${1:-}" != "--json" && "${1:-}" != "--github-output" ]]; then
    eval "$("$PYTHON" "$PYTHON_SCRIPT" --export bash)"
else
    # Delegate to Python for --get, --json, --github-output
    "$PYTHON" "$PYTHON_SCRIPT" "$@"
fi

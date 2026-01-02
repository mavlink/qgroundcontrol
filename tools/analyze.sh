#!/usr/bin/env bash
#
# Run code quality tools on QGroundControl source code
#
# This is a thin wrapper around analyze.py for backward compatibility.
# See analyze.py for full implementation.
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

exec python3 "$SCRIPT_DIR/analyze.py" "$@"

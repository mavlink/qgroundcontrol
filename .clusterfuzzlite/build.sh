#!/usr/bin/env bash
set -euo pipefail
exec uv run --no-project python "$SRC/build.py"

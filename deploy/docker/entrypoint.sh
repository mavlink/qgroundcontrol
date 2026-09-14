#!/usr/bin/env bash
# Docker invokes this through a login shell to load SDK and compiler profiles.
set -euo pipefail
exec /opt/qgc-venv/bin/python /entrypoint.py "$@"

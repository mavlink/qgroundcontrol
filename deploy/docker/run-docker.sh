#!/usr/bin/env sh
# Compatibility entrypoint for existing developer commands and translated guides.
exec python3 "$(dirname "$0")/run_docker.py" build "$@"

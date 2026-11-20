#!/usr/bin/env sh
# Compatibility entrypoint for existing developer commands and translated guides.
# uv supplies the Python named in tools/.python-version; without uv, python3 must be at least that version.
dir="$(dirname "$0")"
if command -v uv >/dev/null 2>&1; then
    exec uv run --no-project --project "$dir/../../tools" python "$dir/run_docker.py" build "$@"
fi
exec python3 "$dir/run_docker.py" build "$@"

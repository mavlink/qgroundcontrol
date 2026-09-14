#!/bin/sh
# Bootstrap the Python package manager used by VM and runner provisioning.
set -eu

if command -v uv >/dev/null 2>&1; then
    exit 0
fi
installer=$(mktemp)
trap 'rm -f "$installer"' EXIT HUP INT TERM
curl --fail --location --retry 3 https://astral.sh/uv/0.11.12/install.sh -o "$installer"
UV_NO_MODIFY_PATH=1 sh "$installer"

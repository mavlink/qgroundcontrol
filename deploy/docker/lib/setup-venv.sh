# shellcheck shell=sh
# Create the qgc venv and install hash-locked build-time Python deps: jinja2/
# httpx/defusedxml for the CMake-invoked generators, aqtinstall for Qt. Synced
# from tools/pyproject.toml [scripts] so the dep list matches developer/runner
# envs. The import smoke-check fails the build on a missing/broken wheel instead
# of crashing later inside a CMake generator.
# Requires uv on PATH and tools/{pyproject.toml,uv.lock} under /tmp/qgc-tools.
# PIP_CMAKE (env) pulls a modern cmake into the venv when the distro's is too old.
set -eu

uv venv --seed /opt/qgc-venv --python python3
VIRTUAL_ENV=/opt/qgc-venv uv sync \
    --project /tmp/qgc-tools \
    --extra scripts --extra qt \
    --no-install-project \
    --frozen --active --no-cache
if [ -n "${PIP_CMAKE:-}" ]; then
    VIRTUAL_ENV=/opt/qgc-venv uv pip install --no-cache "${PIP_CMAKE}"
    /opt/qgc-venv/bin/cmake --version
fi
/opt/qgc-venv/bin/python -c "import jinja2, httpx, defusedxml"
/opt/qgc-venv/bin/aqt --help >/dev/null

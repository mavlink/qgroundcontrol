# shellcheck shell=sh
# Synchronize the same frozen build/Qt profiles used by developer and runner setup.
# The manifest remains in the image so later setup calls can reuse this environment.
# Requires uv on PATH and /opt/qgc-tools/{pyproject.toml,uv.lock}.
set -eu

UV_PROJECT_ENVIRONMENT=/opt/qgc-venv uv sync \
    --project /opt/qgc-tools --python python3 \
    --group scripts --group qt --group build \
    --no-default-groups --no-install-project --frozen
/opt/qgc-venv/bin/python -c "import jinja2, httpx, defusedxml, lxml, fastcrc"
/opt/qgc-venv/bin/aqt --help >/dev/null

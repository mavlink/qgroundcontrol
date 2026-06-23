# shellcheck shell=sh
# Bootstrap apt packages needed before install_dependencies/uv run, plus locale.
# The full build toolchain comes later from install_dependencies; this is only
# what's required to run that script and seed the uv venv over HTTPS.
# APT_EXTRA (env) adds variant packages (e.g. "gcc-12 g++-12" for 22.04).
set -eu

apt-get update
# shellcheck disable=SC2086
apt-get install -y --no-install-recommends \
    ca-certificates \
    git \
    python3 \
    locales \
    ${APT_EXTRA:-}
sed -i 's/^# *\(en_US.UTF-8 UTF-8\)/\1/' /etc/locale.gen
locale-gen
update-locale LANG=en_US.UTF-8
rm -rf /var/lib/apt/lists/*

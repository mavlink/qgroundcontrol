# shellcheck shell=sh
# Bootstrap apt packages needed before install_dependencies/uv run, plus locale.
# The full build toolchain comes later from install_dependencies; this is only
# what's required to run that script and seed the uv venv over HTTPS.
# APT_EXTRA (env) adds variant packages (e.g. "gcc-12 g++-12" for 22.04).
set -eu

# Harden apt against transient mirror outages: retries/timeouts + a geo-routed
# CDN mirror instead of the single archive.ubuntu.com host.
cat > /etc/apt/apt.conf.d/80-retries <<'EOF'
Acquire::Retries "3";
Acquire::http::Timeout "30";
Acquire::https::Timeout "30";
EOF

for f in /etc/apt/sources.list /etc/apt/sources.list.d/ubuntu.sources; do
    [ -f "$f" ] && sed -i 's|https\?://archive.ubuntu.com/ubuntu|mirror://mirrors.ubuntu.com/mirrors.txt|g' "$f"
done

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

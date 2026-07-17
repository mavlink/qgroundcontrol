# shellcheck shell=sh
# Bootstrap apt packages needed before install_dependencies/uv run, plus locale.
# The full build toolchain comes later from install_dependencies; this is only
# what's required to run that script and seed the uv venv over HTTPS.
# APT_EXTRA (env) adds variant packages (e.g. "gcc-12 g++-12" for 22.04).
set -eu

# Harden apt against transient repository outages with transport retries, bounded
# timeouts, and whole-command retries. Minimal Ubuntu images need one signed HTTP
# bootstrap to install the CA bundle before the canonical repositories can use
# HTTPS; apt still authenticates repository metadata during that bootstrap.
. /usr/local/lib/qgc/retry.sh

cat > /etc/apt/apt.conf.d/80-retries <<'EOF'
Acquire::Retries "3";
Acquire::http::Timeout "30";
Acquire::https::Timeout "30";
APT::Update::Error-Mode "any";
EOF

for f in /etc/apt/sources.list /etc/apt/sources.list.d/ubuntu.sources; do
    [ -f "$f" ] && sed -i \
        -e 's|https://archive.ubuntu.com/ubuntu|http://archive.ubuntu.com/ubuntu|g' \
        -e 's|https://security.ubuntu.com/ubuntu|http://security.ubuntu.com/ubuntu|g' \
        "$f"
done

retry apt-get update
retry apt-get install -y --no-install-recommends ca-certificates

for f in /etc/apt/sources.list /etc/apt/sources.list.d/ubuntu.sources; do
    [ -f "$f" ] && sed -i \
        -e 's|http://archive.ubuntu.com/ubuntu|https://archive.ubuntu.com/ubuntu|g' \
        -e 's|http://security.ubuntu.com/ubuntu|https://security.ubuntu.com/ubuntu|g' \
        "$f"
done

retry apt-get update
# shellcheck disable=SC2086
retry apt-get install -y --no-install-recommends \
    git \
    python3 \
    locales \
    ${APT_EXTRA:-}
sed -i 's/^# *\(en_US.UTF-8 UTF-8\)/\1/' /etc/locale.gen
locale-gen
update-locale LANG=en_US.UTF-8
rm -rf /var/lib/apt/lists/*

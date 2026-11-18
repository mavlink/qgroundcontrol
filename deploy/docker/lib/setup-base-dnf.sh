# shellcheck shell=bash
# Seeds the minimal dnf packages + locale to run install_dependencies (--platform
# fedora) and the uv venv; enables RPM Fusion for the gstreamer ugly plugins and
# the libavcodec with H.264 and H.265.
# Fedora's /bin/sh is bash, so pipefail is available even when run as `sh`.
set -euo pipefail

# Harden dnf against transient mirror failures.
printf 'retries=10\ntimeout=30\nmax_parallel_downloads=10\nfastestmirror=True\n' \
    >> /etc/dnf/dnf.conf

dnf -y install ca-certificates git python3 glibc-langpack-en dnf-plugins-core

# RPM Fusion free provides gstreamer1-plugins-ugly and libavcodec-freeworld, whose
# H.264 and H.265 decoders Fedora's libavcodec, which gstreamer1-plugin-libav uses, lacks.
release=$(rpm -E %fedora)
case "${release}" in
    ''|*[!0-9]*) echo "Cannot determine Fedora release (got '${release}')" >&2; exit 1 ;;
esac
dnf -y install \
    "https://download1.rpmfusion.org/free/fedora/rpmfusion-free-release-${release}.noarch.rpm" \
    || echo "warning: RPM Fusion unavailable; the ugly gstreamer plugins and the H.264/H.265 libav decoders will be skipped" >&2

dnf clean all

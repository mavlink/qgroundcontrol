#!/usr/bin/env bash
# Build an aarch64 sysroot inside an amd64 Debian/Ubuntu host via dpkg multiarch.
#
# Faster and more reproducible than debootstrap: no chroot, no qemu, just
# `dpkg --add-architecture arm64` + apt with arch-tagged sources. The libraries
# end up under /usr/lib/aarch64-linux-gnu/ in the host, then we copy/symlink
# into ${SYSROOT} for the cross toolchain's --sysroot=.
#
# Designed for use inside the cross-build Dockerfile; mutates host apt state if run directly.
#
# Env:
#   SYSROOT         Destination sysroot (default: /opt/sysroot)
#   UBUNTU_SUITE    Ubuntu suite to pull arm64 packages from (default: noble)
#   MIRROR          ports.ubuntu.com works for all releases; override only for testing
#
# Idempotent: re-running is safe.

set -euo pipefail

SYSROOT="${SYSROOT:-/opt/sysroot}"
UBUNTU_SUITE="${UBUNTU_SUITE:-noble}"
MIRROR="${MIRROR:-http://ports.ubuntu.com/ubuntu-ports}"

# Reject newlines/junk in overrides — they land verbatim in the apt sources heredoc.
if [[ ! "$UBUNTU_SUITE" =~ ^[a-z][-a-z0-9]*$ ]]; then
    echo "ERROR: UBUNTU_SUITE must match ^[a-z][-a-z0-9]*$: got '$UBUNTU_SUITE'" >&2
    exit 1
fi
if [[ ! "$MIRROR" =~ ^https?://[A-Za-z0-9._/-]+$ ]]; then
    echo "ERROR: MIRROR must be a plain http(s) URL: got '$MIRROR'" >&2
    exit 1
fi

apt_retry() {
    local attempt
    for attempt in 1 2 3; do
        if "$@"; then
            return 0
        fi
        echo "==> apt attempt ${attempt}/3 failed; retrying after ${attempt}0s" >&2
        sleep "${attempt}0"
    done
    "$@"
}

export DEBIAN_FRONTEND=noninteractive

if [[ "$(id -u)" -ne 0 ]]; then
    echo "ERROR: must run as root (dpkg --add-architecture, apt-get install)." >&2
    exit 1
fi

if ! command -v apt-get >/dev/null 2>&1; then
    echo "ERROR: this script targets Debian/Ubuntu hosts (apt-get not found)." >&2
    exit 1
fi

if ! dpkg --print-foreign-architectures | grep -q '^arm64$'; then
    echo "==> Adding arm64 as a foreign dpkg architecture"
    dpkg --add-architecture arm64
fi

# amd64 packages must continue to come from archive.ubuntu.com; only ports
# carries arm64 binaries. Pin the amd64 sources to [arch=amd64] and add a
# separate [arch=arm64] sources file pointing at ports.
configure_apt() {
    local src
    if [[ -f /etc/apt/sources.list ]]; then
        sed -i -E \
            -e 's|^(deb )(http)|\1[arch=amd64] \2|' \
            -e 's|^(deb-src )(http)|\1[arch=amd64] \2|' \
            /etc/apt/sources.list
    fi
    for src in /etc/apt/sources.list.d/*.list; do
        [[ -f "${src}" ]] || continue
        [[ "${src}" == *arm64-ports* ]] && continue
        sed -i -E \
            -e 's|^(deb )(http)|\1[arch=amd64] \2|' \
            -e 's|^(deb-src )(http)|\1[arch=amd64] \2|' \
            "${src}" 2>/dev/null || true
    done

    cat >/etc/apt/sources.list.d/ubuntu-arm64-ports.list <<EOF
deb [arch=arm64] ${MIRROR} ${UBUNTU_SUITE} main restricted universe multiverse
deb [arch=arm64] ${MIRROR} ${UBUNTU_SUITE}-updates main restricted universe multiverse
deb [arch=arm64] ${MIRROR} ${UBUNTU_SUITE}-security main restricted universe multiverse
EOF
}

configure_apt
apt_retry apt-get -o Acquire::Retries=3 update -y --quiet

# Cross compiler + Qt/QGC link deps. Library set mirrors tools/setup/install_dependencies/_packages.py
# DEBIAN_PACKAGES (build/runtime libs); arch tag :arm64 forces target-side install.
apt_retry apt-get -o Acquire::Retries=3 install -y --quiet --no-install-recommends \
    gcc-aarch64-linux-gnu g++-aarch64-linux-gnu \
    pkg-config-aarch64-linux-gnu \
    libc6:arm64 libstdc++6:arm64 libgcc-s1:arm64 \
    libudev-dev:arm64 libinput-dev:arm64 \
    libdrm-dev:arm64 libgbm-dev:arm64 \
    libgl-dev:arm64 libgles-dev:arm64 libegl-dev:arm64 libvulkan-dev:arm64 \
    libfontconfig1-dev:arm64 libfreetype-dev:arm64 libglib2.0-dev:arm64 \
    libx11-dev:arm64 libx11-xcb-dev:arm64 libxext-dev:arm64 libxfixes-dev:arm64 \
    libxcb1-dev:arm64 libxcb-cursor-dev:arm64 libxcb-glx0-dev:arm64 \
    libxcb-icccm4-dev:arm64 libxcb-image0-dev:arm64 libxcb-keysyms1-dev:arm64 \
    libxcb-randr0-dev:arm64 libxcb-render0-dev:arm64 libxcb-render-util0-dev:arm64 \
    libxcb-shape0-dev:arm64 libxcb-shm0-dev:arm64 libxcb-sync-dev:arm64 \
    libxcb-util-dev:arm64 libxcb-xfixes0-dev:arm64 libxcb-xinerama0-dev:arm64 \
    libxcb-xinput-dev:arm64 libxcb-xkb-dev:arm64 \
    libxkbcommon-dev:arm64 libxkbcommon-x11-dev:arm64 \
    libxi-dev:arm64 libxrender-dev:arm64 libxdamage-dev:arm64 libxrandr-dev:arm64 \
    libdbus-1-dev:arm64 libpulse-dev:arm64 libasound2-dev:arm64 libssl-dev:arm64 \
    libnss3-dev:arm64 zlib1g-dev:arm64 libicu-dev:arm64 libpcre2-dev:arm64 \
    libgstreamer1.0-dev:arm64 \
    libgstreamer-plugins-base1.0-dev:arm64 \
    libgstreamer-plugins-bad1.0-dev:arm64 \
    libusb-1.0-0-dev:arm64 libsdl2-dev:arm64

# Optional: not present on every arch/suite combo, do not fail the install.
apt-get install -y --quiet --no-install-recommends gstreamer1.0-qt6:arm64 2>/dev/null || true

HOST_TRIPLE="$(dpkg-architecture -qDEB_HOST_MULTIARCH 2>/dev/null || true)"

mkdir -p "${SYSROOT}/lib" "${SYSROOT}/usr/lib" "${SYSROOT}/usr/include"

for src in /lib/aarch64-linux-gnu /usr/lib/aarch64-linux-gnu; do
    [[ -d "${src}" ]] || continue
    cp -a "${src}" "${SYSROOT}$(dirname "${src}")/"
done

# Exclude the host triple dir so x86_64 internals don't bleed into the arm64 sysroot.
if [[ -d /usr/include ]]; then
    if [[ -n "$HOST_TRIPLE" && -d "/usr/include/$HOST_TRIPLE" ]]; then
        if command -v rsync >/dev/null 2>&1; then
            rsync -a --exclude="/${HOST_TRIPLE}/" /usr/include/ "${SYSROOT}/usr/include/"
        else
            cp -a /usr/include/. "${SYSROOT}/usr/include/"
            rm -rf "${SYSROOT}/usr/include/${HOST_TRIPLE}"
        fi
    else
        cp -a /usr/include/. "${SYSROOT}/usr/include/"
    fi
fi

# Some .pc files reference /usr/lib/aarch64-linux-gnu absolutely (no $prefix);
# pkg-config's PKG_CONFIG_SYSROOT_DIR handles that. No further patching needed.

echo "==> Sysroot ready at ${SYSROOT}"
echo "    cross-gcc: $(aarch64-linux-gnu-gcc --version | head -1)"
echo "    libc6:    $(dpkg-query -W -f='${Version}' libc6:arm64 2>/dev/null || echo unknown)"

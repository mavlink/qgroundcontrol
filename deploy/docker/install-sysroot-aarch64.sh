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

    # deb822 sources (Ubuntu 24.04+): without an explicit arch pin apt requests
    # arm64 from archive.ubuntu.com (amd64/i386 only) and 404s.
    for src in /etc/apt/sources.list.d/*.sources; do
        [[ -f "${src}" ]] || continue
        grep -q '^Architectures:' "${src}" && continue
        sed -i -E '/^Types:/a Architectures: amd64' "${src}" 2>/dev/null || true
    done

    cat >/etc/apt/sources.list.d/ubuntu-arm64-ports.list <<EOF
deb [arch=arm64] ${MIRROR} ${UBUNTU_SUITE} main restricted universe multiverse
deb [arch=arm64] ${MIRROR} ${UBUNTU_SUITE}-updates main restricted universe multiverse
deb [arch=arm64] ${MIRROR} ${UBUNTU_SUITE}-security main restricted universe multiverse
EOF
}

configure_apt
apt_retry apt-get -o Acquire::Retries=3 update -y --quiet

# Target-side libraries are single-sourced from install_dependencies
# (DEBIAN_PACKAGES["cross_arm64"]) so they can't drift from the native build;
# each is arch-tagged :arm64 here. Host cross compilers stay hardcoded — they
# are amd64 packages, not target-side. Override INSTALL_DEPS for non-Docker runs.
INSTALL_DEPS="${INSTALL_DEPS:-/tmp/tools/setup/install_dependencies}"
# read into the array via newlines (no unquoted $(...)) so a glob char in a
# package name can't expand against the filesystem.
arm64_pkgs=()
while IFS= read -r lib; do
    [ -n "${lib}" ] && arm64_pkgs+=("${lib}:arm64")
done < <(python3 "${INSTALL_DEPS}" --print-packages --platform debian --category cross_arm64 | tr ' ' '\n')
if [[ ${#arm64_pkgs[@]} -eq 0 ]]; then
    echo "ERROR: install_dependencies returned no cross_arm64 packages (INSTALL_DEPS=${INSTALL_DEPS})" >&2
    exit 1
fi

# Host cross toolchain (amd64, runs on the build host) — no multiarch conflict.
apt_retry apt-get -o Acquire::Retries=3 install -y --quiet --no-install-recommends \
    gcc-aarch64-linux-gnu g++-aarch64-linux-gnu pkgconf

# Download + unpack the arm64 closure rather than `apt-get install`: installing
# :arm64 -dev packages pulls python3:arm64, which breaks the host python3.
want=("${arm64_pkgs[@]}")
if apt-cache show gstreamer1.0-qt6:arm64 >/dev/null 2>&1; then
    want+=("gstreamer1.0-qt6:arm64")
fi

# Closure members are the unindented lines; keep real arm64 pkgs, drop virtuals/all.
mapfile -t closure < <(
    apt-cache depends --recurse --no-recommends --no-suggests \
        --no-conflicts --no-breaks --no-replaces --no-enhances \
        "${want[@]}" | awk '/^[^[:space:]]/ {print $1}' | grep ':arm64$' | sort -u
)
if [[ ${#closure[@]} -eq 0 ]]; then
    echo "ERROR: empty arm64 dependency closure (apt-cache depends failed?)" >&2
    exit 1
fi

DL_DIR="$(mktemp -d)"
trap 'rm -rf "${DL_DIR}"' EXIT
( cd "${DL_DIR}" && apt_retry apt-get download "${closure[@]}" )

mkdir -p "${SYSROOT}"
shopt -s nullglob
debs=("${DL_DIR}"/*.deb)
if [[ ${#debs[@]} -eq 0 ]]; then
    echo "ERROR: apt-get download produced no .deb files" >&2
    exit 1
fi
for deb in "${debs[@]}"; do
    dpkg-deb -x "${deb}" "${SYSROOT}"
done

echo "==> Sysroot ready at ${SYSROOT} (${#closure[@]} arm64 packages)"
echo "    cross-gcc: $(aarch64-linux-gnu-gcc --version | head -1)"
echo "    libc:      $(find "${SYSROOT}" -name 'libc.so.6' -print -quit 2>/dev/null || echo missing)"

#! /usr/bin/env bash
# Build QGroundControl inside a Multipass VM and copy out the AppImage.
#
# Local use (clones upstream master into the VM):
#   ./deploy/multipass/run-multipass.sh
#
# CI / local-source use (builds the working tree at QGC_SOURCE_DIR, so the VM
# builds the code under test rather than upstream master):
#   QGC_SOURCE_DIR="${PWD}" ./deploy/multipass/run-multipass.sh
#
# Tunables (env): MP_NAME, MP_CPUS, MP_MEM, MP_DISK, MP_IMAGE, OUTPUT_DIR.
set -euo pipefail

NAME="${MP_NAME:-qgc}"
CPUS="${MP_CPUS:-4}"
MEM="${MP_MEM:-8G}"
DISK="${MP_DISK:-25G}"
IMAGE="${MP_IMAGE:-}"                       # empty = Multipass default LTS image
OUTPUT_DIR="${OUTPUT_DIR:-${PWD}}"
SRC="${QGC_SOURCE_DIR:-}"
GUEST_REPO="qgroundcontrol"                 # relative to the VM user's home

cleanup() { multipass delete --purge "${NAME}" >/dev/null 2>&1 || true; }
trap cleanup EXIT

# shellcheck disable=SC2086  # IMAGE is an optional positional arg; intentional split.
multipass launch ${IMAGE:+"${IMAGE}"} \
    --name "${NAME}" --cpus "${CPUS}" --memory "${MEM}" --disk "${DISK}"

if [[ -n "${SRC}" ]]; then
    # Tarball + `multipass transfer`: `multipass mount` needs the multipass-sshfs snap (often
    # unreachable on CI) and tar over `multipass exec` truncates large streams.
    # Stage under $HOME, not /tmp: the multipass snap is strict-confined with a
    # private /tmp, so it can only read the tarball via its `home` interface.
    TARBALL="$(mktemp -p "${HOME}" qgc-src-XXXXXX.tar.gz)"
    tar -C "${SRC}" -czf "${TARBALL}" .
    multipass transfer "${TARBALL}" "${NAME}:/tmp/qgc-src.tar.gz"
    rm -f "${TARBALL}"
    multipass exec "${NAME}" -- mkdir -p "${GUEST_REPO}"
    multipass exec "${NAME}" -- tar -C "${GUEST_REPO}" -xzf /tmp/qgc-src.tar.gz
else
    multipass exec "${NAME}" -- \
        git clone https://github.com/mavlink/qgroundcontrol.git "${GUEST_REPO}" --recurse-submodules
fi

multipass exec "${NAME}" -- bash "${GUEST_REPO}/deploy/multipass/build-in-vm.sh"

# build-in-vm.sh writes the resolved AppImage path here (absolute, VM-local).
APPIMAGE="$(multipass exec "${NAME}" -- cat qgc-appimage-path)"
multipass transfer "${NAME}:${APPIMAGE}" "${OUTPUT_DIR}/"

echo "Output: ${OUTPUT_DIR}/$(basename "${APPIMAGE}")"

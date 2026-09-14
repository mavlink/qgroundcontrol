#!/usr/bin/env bash
# Build QGroundControl from inside a provisioned VM (Multipass / Vagrant-libvirt).
# Expects the repo checked out (or mounted) at this script's ../.. and a network
# connection for aqtinstall. Produces $BUILD_DIR/QGroundControl-<arch>.AppImage.
#
# Tunables (env):
#   BUILD_DIR   VM-local build dir (default: $HOME/qgc-build; kept off any host
#               mount so build I/O doesn't cross 9p/virtfs).
#   BUILD_TYPE  CMake build type (default: Release).
#   JOBS        Parallel build jobs (default: all cores via --parallel).
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${HOME}/qgc-build}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
QT_OUT="${QT_OUT:-${HOME}/Qt}"

cd "${REPO}"

export PATH="${HOME}/.local/bin:${PATH}"
sh tools/setup/install_uv.sh
python3 tools/setup/install_python.py build,qt
export PATH="${REPO}/tools/.venv/bin:${PATH}"
python tools/setup/install_dependencies --platform debian

QT_VERSION="$(python tools/setup/read_config.py --get qt.version)"
QT_MODULES="$(python tools/setup/read_config.py --get qt.modules)"
python tools/setup/install_qt.py install \
    --version "${QT_VERSION}" --host linux --arch linux_gcc_64 \
    --modules "${QT_MODULES}" --outdir "${QT_OUT}"
# linux_gcc_64 -> on-disk dir gcc_64 (resolve_arch_dir strips the linux_ prefix).
QT_ROOT="${QT_OUT}/${QT_VERSION}/gcc_64"

"${QT_ROOT}/bin/qt-cmake" -S "${REPO}" -B "${BUILD_DIR}" -G Ninja \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DQGC_BUILD_TESTING=OFF \
    -DQGC_STABLE_BUILD=OFF \
    -DCMAKE_PREFIX_PATH="${QT_ROOT}"
build_jobs=(--parallel)
[[ -n "${JOBS:-}" ]] && build_jobs=(--parallel "${JOBS}")
cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" "${build_jobs[@]}"
cmake --install "${BUILD_DIR}" --config "${BUILD_TYPE}"

appimages=("${BUILD_DIR}"/*.AppImage)
echo "AppImage(s) produced:"
printf '%s\n' "${appimages[@]}"

# Hand the resolved path back to the host orchestrator (run-multipass.sh) so the
# transfer doesn't hardcode BUILD_DIR/arch.
printf '%s\n' "${appimages[0]}" > "${HOME}/qgc-appimage-path"

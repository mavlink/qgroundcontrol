#!/usr/bin/env bash
set -euo pipefail

readonly source_dir=/tmp/qgroundcontrol-runner-image
readonly qt_prefix=/opt/qgc-sdk

cleanup() {
    rm -rf -- "${source_dir}"
}
trap cleanup EXIT

if [[ ! "${QGC_SOURCE_REF}" =~ ^[0-9a-f]{40}$ ]]; then
    echo "QGC_SOURCE_REF must be a full Git commit SHA" >&2
    exit 1
fi

sudo apt-get -o Acquire::Retries=3 update -qq
sudo apt-get -o Acquire::Retries=3 install -y -qq --no-install-recommends \
    ca-certificates git python3 python3-venv

git clone --filter=blob:none --no-checkout "${QGC_SOURCE_REPOSITORY}" "${source_dir}"
git -C "${source_dir}" fetch --depth 1 origin "${QGC_SOURCE_REF}"
git -C "${source_dir}" checkout --detach FETCH_HEAD

python3 "${source_dir}/tools/setup/install_dependencies" --platform debian

python3 -m venv "${source_dir}/.venv"
sudo mkdir -p "${qt_prefix}/Qt"
sudo chown -R "$(id -u):$(id -g)" "${qt_prefix}"

PATH="${source_dir}/.venv/bin:${PATH}" \
    "${source_dir}/.venv/bin/python" "${source_dir}/tools/setup/install_qt.py" install \
    --version "${QGC_QT_VERSION}" \
    --host linux \
    --target desktop \
    --arch linux_gcc_64 \
    --modules "${QGC_QT_MODULES}" \
    --outdir "${qt_prefix}/Qt"

readonly qt_root="${qt_prefix}/Qt/${QGC_QT_VERSION}/gcc_64"
printf '%s\n' "${QGC_QT_MODULES}" >"${qt_root}/.qgc-modules"

sudo chown -R root:root "${qt_prefix}"
sudo chmod -R a+rX "${qt_prefix}"
echo "QGC_PREINSTALLED_QT_DIR=${qt_prefix}" | sudo tee -a /etc/environment >/dev/null
sudo tee /etc/profile.d/qgc-sdk.sh >/dev/null <<EOF
export QGC_PREINSTALLED_QT_DIR=${qt_prefix}
EOF

sudo apt-get clean
sudo rm -rf -- /var/lib/apt/lists/*

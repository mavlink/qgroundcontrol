#!/usr/bin/env bash
set -euo pipefail

readonly source_dir=/tmp/qgroundcontrol-runner-image
readonly qt_prefix=/opt/qgc-sdk

sudo apt-get update -qq
sudo apt-get install -y -qq git python3-pip python3-venv software-properties-common
sudo add-apt-repository -y universe
sudo apt-get update -qq

git clone --filter=blob:none --no-checkout "${QGC_SOURCE_REPOSITORY}" "${source_dir}"
git -C "${source_dir}" fetch --depth 1 origin "${QGC_SOURCE_REF}"
git -C "${source_dir}" checkout --detach FETCH_HEAD

mapfile -t packages < <(
    python3 "${source_dir}/tools/setup/install_dependencies" \
        --platform debian \
        --print-packages \
        | tr ' ' '\n' \
        | sed '/^$/d'
)
sudo apt-get install -y -qq "${packages[@]}"

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
    --aqt-source "${QGC_AQT_SOURCE}" \
    --outdir "${qt_prefix}/Qt"

sudo chmod -R a+rX "${qt_prefix}"
echo "QGC_PREINSTALLED_QT_DIR=${qt_prefix}" | sudo tee -a /etc/environment >/dev/null
sudo tee /etc/profile.d/qgc-sdk.sh >/dev/null <<EOF
export QGC_PREINSTALLED_QT_DIR=${qt_prefix}
EOF

rm -rf "${source_dir}"
sudo apt-get clean
sudo rm -rf /var/lib/apt/lists/*

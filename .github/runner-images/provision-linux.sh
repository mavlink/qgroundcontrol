#!/usr/bin/env bash
set -euo pipefail

readonly source_dir=/tmp/qgroundcontrol-runner-image

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
    ca-certificates curl git python3 python3-venv

git clone --filter=blob:none --no-checkout "${QGC_SOURCE_REPOSITORY}" "${source_dir}"
git -C "${source_dir}" fetch --depth 1 origin "${QGC_SOURCE_REF}"
git -C "${source_dir}" checkout --detach FETCH_HEAD

python3 "${source_dir}/.github/runner-images/provision_linux.py"

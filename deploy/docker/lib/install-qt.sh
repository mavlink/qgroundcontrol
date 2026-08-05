# shellcheck shell=sh
# Install one Qt build via aqtinstall, retried. Version + module list come from
# build-config.json (read_config.py); everything lands under /opt/Qt.
# Usage: install-qt.sh <host_os> <target> <arch> [extra aqt args...]
# Requires retry.sh, the qgc venv (aqt), and /tmp/tools/setup/read_config.py.
set -eu

. /usr/local/lib/qgc/retry.sh

qt_version=$(python3 /tmp/tools/setup/read_config.py --get qt.version)
qt_modules=$(python3 /tmp/tools/setup/read_config.py --get qt.modules)

host_os=$1
target=$2
arch=$3
shift 3

mkdir -p /opt/Qt
# qt_modules is a space-separated list that must word-split into -m args.
# shellcheck disable=SC2086
retry /opt/qgc-venv/bin/aqt install-qt "${host_os}" "${target}" "${qt_version}" "${arch}" \
    -O /opt/Qt -m ${qt_modules} "$@"

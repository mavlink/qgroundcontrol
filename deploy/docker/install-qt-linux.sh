#!/usr/bin/env bash

# Set default variables
QT_VERSION="${QT_VERSION:-5.15.2}"
QT_PATH="${QT_PATH:-/opt/Qt}"
QT_HOST="${QT_HOST:-linux}"
QT_TARGET="${QT_TARGET:-desktop}"
QT_MODULES="${QT_MODULES:-qtcharts}"

# Exit immediately if a command exits with a non-zero status
set -e

apt update
apt install python3 python3-pip -y
pip3 install aqtinstall
aqt install --outputdir ${QT_PATH} ${QT_VERSION} ${QT_HOST} ${QT_TARGET} -m ${QT_MODULES}
echo "Remember to export the following to your PATH: ${QT_PATH}/${QT_VERSION}/*/bin"
echo "export PATH=$(readlink -e ${QT_PATH}/${QT_VERSION}/*/bin/):PATH"
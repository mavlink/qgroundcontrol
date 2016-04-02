#!/bin/sh
HERE="$(dirname "$(readlink -f "${0}")")"
export LD_LIBRARY_PATH="${HERE}/Qt/libs":$LD_LIBRARY_PATH
export QML2_IMPORT_PATH="${HERE}/Qt/qml"
export QT_PLUGIN_PATH="${HERE}/Qt/plugins"
"${HERE}/qgroundcontrol" "$@"

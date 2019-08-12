#!/bin/sh
HERE="$(dirname "$(readlink -f "${0}")")"
export LD_LIBRARY_PATH="${HERE}/usr/lib/x86_64-linux-gnu":"${HERE}/Qt/libs":$LD_LIBRARY_PATH
export QML2_IMPORT_PATH="${HERE}/Qt/qml"
export QT_PLUGIN_PATH="${HERE}/Qt/plugins"

# hack until icon issue with AppImage is resolved
mkdir -p ~/.icons && \cp -f ${HERE}/qgroundcontrol.png ~/.icons

"${HERE}/CustomQGC" "$@"

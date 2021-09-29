#!/bin/sh
HERE="$(dirname "$(readlink -f "${0}")")"
export LD_LIBRARY_PATH="${HERE}/Qt/libs":$LD_LIBRARY_PATH

# hack until icon issue with AppImage is resolved
mkdir -p ~/.icons && cp ${HERE}/qgroundcontrol.png ~/.icons

"${HERE}/QGroundControl" "$@"

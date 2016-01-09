#!/bin/sh
export LD_LIBRARY_PATH=`pwd`/libs:$LD_LIBRARY_PATH
export QML_IMPORT_PATH=`pwd`/libs/qml
export QML2_IMPORT_PATH=`pwd`/libs/qml
export QT_QPA_PLATFORM_PLUGIN_PATH=`pwd`/libs/platforms
./qgroundcontrol "$@"

#!/bin/sh
export LD_LIBRARY_PATH=`pwd`/Qt/libs:$LD_LIBRARY_PATH
export QML2_IMPORT_PATH=`pwd`/Qt/qml
export QT_PLUGIN_PATH=`pwd`/Qt/plugins
./qgroundcontrol "$@"

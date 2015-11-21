#!/bin/sh
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$HOME/Qt/5.5/gcc_64/lib
export QML_IMPORT_PATH=$HOME/Qt/5.5/gcc_64/qml/
export QML2_IMPORT_PATH=$HOME/Qt/5.5/gcc_64/qml/
export QT_QPA_PLATFORM_PLUGIN_PATH=$HOME/Qt/5.5/gcc_64/plugins/platforms/
./qgroundcontrol "$@"

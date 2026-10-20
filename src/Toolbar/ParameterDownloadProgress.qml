import QtQuick

import QGroundControl

/// Provides UI for parameter download progress. This is overlayed on top of the FlyViewToolBar.

Item {
    id: control

    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    Rectangle {
        anchors.bottom: parent.bottom
        height:         control.height * 0.05
        width:          _activeVehicle ? _activeVehicle.loadProgress * parent.width : 0
        color:          QGroundControl.globalPalette.colorGreen
    }
}

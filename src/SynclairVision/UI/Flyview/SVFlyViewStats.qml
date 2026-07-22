import QtQuick

import QGroundControl
import QGroundControl.Controls

Item {
    id: root
    height: 200
    width: 600

    property var open

    QGCPalette { id: qgcPalette }

    SVBackground {
        id: background
        anchors.fill: parent

        radius: SVUnits.radius
        borderColor: qgcPalette.windowShade
        borderWidth: 1
    }




}
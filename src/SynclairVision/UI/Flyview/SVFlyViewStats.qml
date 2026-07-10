import QtQuick

import QGroundControl
import QGroundControl.Controls

Item {
    id: root

    property var open

    QGCPalette { id: qgcPalette }

    Rectangle {
        id: test
        anchors.fill: parent
        width: 500
        height: 300

        color: qgcPalette.windowTransparent
        visible: open
    }

    Rectangle {
        id: button
        width: SVUnits.objectWidth / 2
        height: SVUnits.objectWidth
        anchors.right: (open) ? test.left : parent.right
        anchors.verticalCenter: test.verticalCenter
        color: qgcPalette.windowTransparent

        SVArrow {
            
        }
    }


}
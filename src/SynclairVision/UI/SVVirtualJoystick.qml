import QtQuick
import QtQuick.Shapes 2.15

import QGroundControl


Item {
    id: root

    QGCPalette { id: qgcPalette }

    Rectangle {
        id: border
        anchors.fill: parent
        radius: width / 2
        color: qgcPalette.window
    }

    SVJoystickArea {
        id: joystick
        anchors.fill: parent
        anchors.margins: 10
    }
}
import QtQuick
import QtQuick.Shapes 2.15

import QGroundControl

Item {
    id: root

    property int borderWidth: 0

    QGCPalette { id: qgcPalette}

    implicitWidth: joystick.width + zoom.width - borderWidth
    implicitHeight: Math.max(joystick.height, zoom.height)

    SVVirtualJoystick {
        id: joystick
        height: 200
        width: height
        x: 0
        y: 0
        borderWidth: root.borderWidth
    }

    SVZoom {
        id: zoom
        height: 140
        width: height / 2
        x: joystick.width - borderWidth
        y: root.implicitHeight - height
        borderWidth: root.borderWidth
    }
}
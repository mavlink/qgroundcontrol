import QtQuick

import QGroundControl
import QGroundControl.Controls

Rectangle {
    id: root

    antialiasing: true

    property real startX: 0
    property real startY: 0
    property real endX: 0
    property real endY: 0
    property real thickness: 1

    x: startX - Math.floor(thickness / 2)
    y: startY - Math.floor(thickness / 2)
    width: Math.sqrt(Math.pow(endX - startX, 2) + Math.pow(endY - startY, 2))
    height: thickness
    color: "white"

    rotation: Math.atan2(endY - startY, endX - startX) * 180 / Math.PI
    transformOrigin: Item.Left
}

import QtQuick
import QtQuick.Shapes 2.15

Item {
    id: root

    width: 230
    height: 230

    SVVirtualJoystick {
        height: root.height
        width: root.height

        anchors.left: root.left


    }
}
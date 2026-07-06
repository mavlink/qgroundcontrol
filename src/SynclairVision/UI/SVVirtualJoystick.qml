import QtQuick
import QtQuick.Shapes 2.15

import QGroundControl


Item {
    id: root

    property string joystickType: "Standard"
    property int borderWidth

    QGCPalette { id: qgcPalette }

    Rectangle {
        id: border
        anchors.fill: parent
        radius: width / 2
        color: qgcPalette.window
    }

    Loader {
        id: joystickLoader
        anchors.fill: parent
        anchors.margins: borderWidth
        sourceComponent: (root.joystickType === "Drag") ? dragComponent : areaComponent

        onLoaded: {
            if(root.joystickType === "Simple") {
                item.hasInnerRing = false
            }

        }
    }

    Component {
        id: areaComponent
        SVJoystickArea {
            id: joystick
            anchors.fill: parent
            //anchors.margins: borderWidth
        }
    }

    Component {
        id: dragComponent
        SVJoystickDrag {
            id: joystick
            anchors.fill: parent
            //anchors.margins: borderWidth
        }
    }
}
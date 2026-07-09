import QtQuick
import QtQuick.Shapes 2.15

import QGroundControl


Item {
    id: root

    property string joystickType: "standard"

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
        anchors.margins: SVUnits.margin

        sourceComponent: {
            if(root.joystickType === "standard") {
                return standardComponent
            } else if(root.joystickType === "simple") {
                return simpleComponent
            } else if(root.joystickType === "drag") {
                return dragComponent
            }
        }
    }

    Component {
        id: standardComponent
        SVJoystickArea {
            hasInnerRing: true
        }
    }

    Component {
        id: simpleComponent
        SVJoystickArea {
            hasInnerRing: false
        }
    }

    Component {
        id: dragComponent
        SVJoystickDrag {}
    }
}

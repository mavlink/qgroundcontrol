import QtQuick
import QtQuick.Shapes 2.15

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView
import QGroundControl.FlightMap

//import QGroundControl.SynclairVision

Item {
    id: root

    property int borderWidth: 5

    QGCPalette { id: qgcPalette}

    implicitWidth: content.width
    implicitHeight: Math.max(joystick.height, zoom.height)

    Item {
        id: content
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        width: joystick.width + zoom.width - borderWidth
        height: Math.max(joystick.height, zoom.height)
        opacity: (SVSettings.controlPanelPassiveOpacity && (!contentHoverHandler.hovered || !(!SVState.lockControls && SVState.cameraSelected !== -1))
                  && !SVState.shortcutJoystickHeld.some(held => held) && !SVState.shortcutZoomInHeld
                  && !SVState.shortcutZoomOutHeld && !SVState.shortcutSmallMovementHeld)
                  ? SVSettings.controlPanelPassiveOpacityValue
                  : 1

        Behavior on opacity {
            NumberAnimation {
                duration: 250
            }
        }

    

        HoverHandler {
            id: contentHoverHandler
        }

        SVJoystick {
            id: joystick
            height: SVSettings.joystickSize / 4 * SVUnits.height
            width: height
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            joystickType: SVSettings.joystickType
        }

        SVZoom {
            id: zoom
            height: SVSettings.zoomSize / 4 * SVUnits.height
            width: height / 2
            anchors.left: joystick.right

            anchors.top: (SVSettings.controlPanelPosition === "Top-center") ? parent.top : undefined
            anchors.bottom: (SVSettings.controlPanelPosition === "Top-center") ? undefined : parent.bottom
        }
    }
}

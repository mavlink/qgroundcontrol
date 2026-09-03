import QtQuick

import QGroundControl
import QGroundControl.Controls

Item {
    id: root

    property int detectionViewId

    readonly property var digiview: QGroundControl.digiviewManager
    readonly property bool detectionEnabled: !!digiview
        && digiview.hasVideoOutputParameters
        && digiview.videoOutputStreamName === digiview.streamName
        && digiview.videoOutputDetectionOverlayMode !== DigiviewProtocol.DetectionOverlayNone

    function setDetectionTracking() {
        const cameraSlot = SVState.cameraSelected

        if (!detectionEnabled || cameraSlot < 0 || !digiview) {
            return
        }

        digiview.setDetectionTracking(cameraSlot, root.detectionViewId, false)
    }

    function clearDetectionTracking() {
        const cameraSlot = SVState.cameraSelected

        if (!detectionEnabled || cameraSlot < 0 || !digiview) {
            return
        }

        digiview.clearDetectionTracking(cameraSlot)
    }

    z: mouseArea.containsMouse ? 100 : 0

    SVBackground {
        anchors.fill: parent
        //anchors.margins: root.enabled ? 0 : SVUnits.lineWidth * 2
        
        transparentBackground: true
        enabled: true
        hoverEnabled: true
        hovered: mouseArea.containsMouse
        //checkable: true
        pressed: mouseArea.pressed
        hoverPosition: Qt.point(mouseArea.mouseX, mouseArea.mouseY)
        opacity: 0.5
    }

    SVBorder {
        anchors.fill: parent
                anchors.margins: 2

        borderVisible: mouseArea.containsMouse || mouseArea.pressed
        borderWidth: 1
        borderColor: "white"
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: root.enabled
        hoverEnabled: true

        onClicked: (mouse) => {
            if (mouse.button === Qt.RightButton) {
                root.clearDetectionTracking()
                SVState.activateSttTracking()

            } else if (mouse.button === Qt.LeftButton) {
                root.setDetectionTracking()
            }
        }
    }

}

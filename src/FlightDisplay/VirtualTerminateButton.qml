import QtQuick 2.3
import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Vehicle       1.0

Item {
    id: virtualTerminateButton

    property var   _activeVehicle:            QGroundControl.multiVehicleManager.activeVehicle
    property bool  isTerminated: _activeVehicle.terminated()

    signal terminateRequest

    Rectangle {
        width: parent.width
        height: parent.height
        color: "black"

        Image {
            id: virtualTerminateButtonImage
            source: isTerminated ? "/res/TerminateButtonPressed.png" : "/res/TerminateButtonNotPressed.png"
            anchors.fill: parent
            // fillMode: Image.PreserveAspectFit
            fillMode: Image.Stretch
            smooth: true
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                if (!isTerminated) {
                    virtualTerminateButton.terminateRequest()
                }
            }
        }

    }
}
import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.FlightDisplay

AnalyzePage {
    id: root
    pageComponent: pageComponent
    pageDescription: qsTr("Raw Video Test Page")

    Component {
        id: pageComponent

        Item {
            anchors.fill: parent

            // Start video only when vehicle exists
            Connections {
                target: QGroundControl.multiVehicleManager
                function onActiveVehicleChanged() {
                    if (QGroundControl.multiVehicleManager.activeVehicle &&
                        !QGroundControl.videoManager.streaming) {
                        QGroundControl.videoManager.startVideo()
                    }
                }
            }

            // RAW VIDEO SURFACE — NOTHING ON TOP OF IT
            QGCVideoBackground {
                id: video
                anchors.fill: parent
                visible: QGroundControl.videoManager.decoding
            }
        }
    }
}

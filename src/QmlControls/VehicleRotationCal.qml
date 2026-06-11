import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls

Rectangle {
    id: root

    // Calibration state of the orientation shown by this control.
    // Values must stay in sync with SensorsComponentController::SideCalState.
    enum CalState {
        Idle,       ///< No calibration data to show (neutral preview)
        Incomplete, ///< Calibration running, this orientation not yet calibrated
        InProgress, ///< This orientation actively being calibrated
        Completed   ///< This orientation calibration complete
    }

    property int calState: VehicleRotationCal.CalState.Idle

    // Text to show while calibration is in progress
    property string calInProgressText: qsTr("Hold Still")

    // Image source
    property var imageSource: ""

    color: qgcPal.text

    states: [
        State {
            name: "idle"
            when: root.calState === VehicleRotationCal.CalState.Idle
            PropertyChanges {
                root.color: qgcPal.text
                statusLabel.text: ""
            }
        },
        State {
            name: "incomplete"
            when: root.calState === VehicleRotationCal.CalState.Incomplete
            PropertyChanges {
                root.color: "red"
                statusLabel.text: qsTr("Incomplete")
            }
        },
        State {
            name: "inProgress"
            when: root.calState === VehicleRotationCal.CalState.InProgress
            PropertyChanges {
                root.color: "yellow"
                statusLabel.text: root.calInProgressText
            }
        },
        State {
            name: "completed"
            when: root.calState === VehicleRotationCal.CalState.Completed
            PropertyChanges {
                root.color: "green"
                statusLabel.text: qsTr("Completed")
            }
        }
    ]

    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    Rectangle {
        readonly property int inset: 5

        x:      inset
        y:      inset
        width:  parent.width - (inset * 2)
        height: parent.height - (inset * 2)
        color: qgcPal.windowShade

        Image {
            width:      parent.width
            height:     parent.height
            source:     root.imageSource
            fillMode:   Image.PreserveAspectFit
            smooth: true
        }

        QGCLabel {
            id:                     statusLabel
            width:                  parent.width
            height:                 parent.height
            horizontalAlignment:    Text.AlignHCenter
            verticalAlignment:      Text.AlignBottom
            font.pointSize:         ScreenTools.mediumFontPointSize
        }
    }
}

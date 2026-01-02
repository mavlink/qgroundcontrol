import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

PX4TuningComponent {
    model: ListModel {
        ListElement {
            buttonText: qsTr("Rate Controller")
            tuningPage: "PX4TuningComponentPlaneRate.qml"
        }
        ListElement {
            buttonText: qsTr("Rate Controller")
            tuningPage: "PX4TuningComponentPlaneAttitude.qml"
        }
    }
}

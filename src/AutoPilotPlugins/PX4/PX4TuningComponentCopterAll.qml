import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

PX4TuningComponent {
    model: ListModel {
        ListElement {
            buttonText: qsTr("Rate Controller")
            tuningPage: "PX4TuningComponentCopterRate.qml"
        }
        ListElement {
            buttonText: qsTr("Attitude Controller")
            tuningPage: "PX4TuningComponentCopterAttitude.qml"
        }
        ListElement {
            buttonText: qsTr("Velocity Controller")
            tuningPage: "PX4TuningComponentCopterVelocity.qml"
        }
        ListElement {
            buttonText: qsTr("Position Controller")
            tuningPage: "PX4TuningComponentCopterPosition.qml"
        }
    }
}

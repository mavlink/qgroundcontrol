import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls

PX4TuningComponent {
    model: ListModel {
        ListElement {
            buttonText: qsTr("Multirotor")
            tuningPage: "PX4TuningComponentCopterAll.qml"
        }
        //ListElement {
        //    buttonText: qsTr("Fixed Wing")
        //    tuningPage: "PX4TuningComponentPlaneAll.qml"
        //}
    }
}

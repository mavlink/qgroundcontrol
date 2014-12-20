import QtQuick 2.2
import QtQuick.Controls 1.2
import QGroundControl.FactSystem 1.0

Item {
    TextInput {
        objectName: "testControl"
        text: autopilot.parameters["RC_MAP_THROTTLE"].value
        font.family: "Helvetica"
        font.pointSize: 24
        color: "red"
        focus: true
        onAccepted: { autopilot.parameters["RC_MAP_THROTTLE"].value = text; }
    }
}
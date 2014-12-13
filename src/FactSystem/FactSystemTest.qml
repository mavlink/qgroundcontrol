import QtQuick 2.2
import QtQuick.Controls 1.2
import QGroundControl.FactSystem 1.0
import QGroundControlFactControls 1.0

Item {
    TextInput {
        objectName: "testControl"
        text: parameters["RC_MAP_THROTTLE"].value
        font.family: "Helvetica"
        font.pointSize: 24
        color: "red"
        focus: true
        onAccepted: { parameters["RC_MAP_THROTTLE"].value = text; }
    }
}
import QtQuick

import QGroundControl
import QGroundControl.Controls

/// Switch that records the receiver's raw input and output, with the file of the connection being recorded.
GPSField {
    id: root

    required property GPSReceiver receiver

    label: qsTr("Record receiver data")

    QGCCheckBoxSlider {
        objectName: "rtkRecordReceiverData"
        Accessible.name: root.label
        checked: root.receiver.recordSessions
        onClicked: root.receiver.recordSessions = checked
    }

    GPSNoteLabel {
        objectName: "rtkRecordingFile"
        visible: text !== ""
        text: root.receiver.recordingFile !== ""
              ? qsTr("Recording file: %1").arg(root.receiver.recordingFile)
              : root.receiver.recordSessions ? qsTr("Recording starts with the next connection.") : ""
    }
}

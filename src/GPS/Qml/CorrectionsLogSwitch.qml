pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GPS

/// Switch that logs the correction stream sent to vehicles, with the file being written.
ColumnLayout {
    id: root

    property GPSCorrectionManager corrections: QGroundControl.gpsManager.corrections

    Layout.minimumWidth: 0
    spacing: ScreenTools.defaultFontPixelHeight / 4

    QGCCheckBoxSlider {
        objectName: "correctionsLogSwitch"
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        text: qsTr("Log corrections to a file")
        checked: root.corrections.logToFile
        onClicked: root.corrections.logToFile = checked
    }

    QGCNoteLabel {
        objectName: "correctionsLogFile"
        visible: text !== ""
        text: root.corrections.logFile !== "" ? qsTr("Log file: %1").arg(root.corrections.logFile) : ""
    }
}

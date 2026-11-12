pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GPS

/// What the correction stream carries for an RTK fix: the constellations it has observations of, with their satellites
/// where the stream tells, and a warning when a vehicle cannot get a fix from it or cannot use all of it.
ColumnLayout {
    id: root

    required property GPSCorrectionManager corrections

    spacing: ScreenTools.defaultFontPixelHeight / 4
    visible: root.corrections.streamContent !== "" || root.corrections.streamWarning !== ""

    GPSWrappingRow {
        objectName: "correctionsStreamContent"
        Layout.fillWidth: true
        visible: root.corrections.streamContent !== ""
        //: Label of the satellite constellations, such as GPS and Galileo, that the corrections have observations of
        label: qsTr("Constellations")
        labelText: root.corrections.streamContent
        labelWrapMode: Text.Wrap
    }

    QGCNoteLabel {
        objectName: "correctionsStreamWarning"
        visible: root.corrections.streamWarning !== ""
        text: root.corrections.streamWarning
        warning: true
    }
}

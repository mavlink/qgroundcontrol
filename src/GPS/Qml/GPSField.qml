pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// A label above its control, which keeps the GPS panels usable in the narrow indicator drawer. An optional note
/// goes below the control.
ColumnLayout {
    id: root

    property string label
    /// Explanation below the control; children declared in a field also go below it.
    property string note
    default property alias content: control.data

    Layout.minimumWidth: 0
    spacing: ScreenTools.defaultFontPixelHeight / 4

    GPSNoteLabel { text: root.label }

    ColumnLayout {
        id: control
        Layout.minimumWidth: 0
        spacing: root.spacing
    }

    GPSNoteLabel {
        visible: text !== ""
        text: root.note
    }
}

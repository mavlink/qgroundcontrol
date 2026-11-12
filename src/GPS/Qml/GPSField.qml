pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// A label above its control, which keeps the GPS panels usable on a narrow settings page. An optional note goes
/// below the control.
ColumnLayout {
    id: root

    property string label
    /// Explanation below the control; children declared in a field also go below it.
    property string note
    default property alias content: control.data

    Layout.minimumWidth: 0
    spacing: ScreenTools.defaultFontPixelHeight / 4

    QGCNoteLabel { text: root.label }

    ColumnLayout {
        id: control
        Layout.minimumWidth: 0
        spacing: root.spacing
    }

    QGCNoteLabel {
        visible: text !== ""
        text: root.note
    }
}

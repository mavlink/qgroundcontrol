import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Wrapping plain-text note of the GPS panels; it takes the width its layout gives it instead of widening the layout.
QGCLabel {
    id: control

    /// Shows the note in the palette's warning color.
    property bool warning: false

    Layout.fillWidth: true
    Layout.minimumWidth: 0
    Layout.preferredWidth: 0
    wrapMode: Text.Wrap
    textFormat: Text.PlainText
    color: warning ? notePalette.warningText : notePalette.text

    QGCPalette { id: notePalette; colorGroupEnabled: control.enabled }
}

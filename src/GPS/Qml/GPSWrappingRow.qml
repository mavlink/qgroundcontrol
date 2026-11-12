import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// A label and its value, which wraps instead of widening the panel or running past it: a sentence, or a value
/// reported from outside, such as a long endpoint.
RowLayout {
    id: root

    property alias label: rowLabel.text
    property alias labelText: rowValue.text
    /// Text.WrapAnywhere breaks a value without spaces; a sentence takes Text.Wrap, which breaks between words.
    property alias labelWrapMode: rowValue.wrapMode

    spacing: ScreenTools.defaultFontPixelWidth * 2

    QGCLabel {
        id: rowLabel
        textFormat: Text.PlainText
    }

    QGCNoteLabel {
        id: rowValue
        horizontalAlignment: Text.AlignRight
        wrapMode: Text.WrapAnywhere
    }
}

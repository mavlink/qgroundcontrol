import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

RowLayout {
    property alias label:                   _labelLabel.text
    property alias labelText:              _label.text
    property alias labelTextFormat:        _label.textFormat
    property alias labelTextElide:         _label.elide
    property real  labelPreferredWidth:    -1
    /// Widest the text gets; with labelTextElide, longer text is elided to fit.
    property real  labelMaximumWidth:      Number.POSITIVE_INFINITY

    spacing: ScreenTools.defaultFontPixelWidth * 2

    QGCLabel {
        id:                  _labelLabel
        Layout.fillWidth:    true
        Layout.minimumWidth: implicitWidth
    }

    QGCLabel {
        id:                     _label
        Layout.preferredWidth:  labelPreferredWidth
        Layout.maximumWidth:    labelMaximumWidth
    }
}

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

RowLayout {
    property alias label:                   _labelLabel.text
    property alias labelText:              _label.text
    property real  labelPreferredWidth:    -1

    spacing: ScreenTools.defaultFontPixelWidth * 2

    QGCLabel {
        id:                 _labelLabel
        Layout.fillWidth:   true
    }

    QGCLabel {
        id:                     _label
        Layout.preferredWidth:  labelPreferredWidth
    }
}

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

RowLayout {
    property string label:                   fact.shortDescription
    property alias  fact:                    _factTextField.fact
    property real   textFieldPreferredWidth: -1
    property alias  textFieldUnitsLabel:     _factTextField.unitsLabel
    property alias  textFieldShowUnits:      _factTextField.showUnits
    property alias  textFieldShowHelp:       _factTextField.showHelp
    property alias  textField:               _factTextField

    spacing: ScreenTools.defaultFontPixelWidth * 2

    QGCLabel {
        Layout.fillWidth:   true
        text:               label
        visible:            label !== ""
    }

    FactTextField {
        id:                     _factTextField
        Layout.preferredWidth:  textFieldPreferredWidth
    }
}

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

GridLayout {
    property string label:                   fact ? fact.shortDescription : ""
    property alias  fact:                    _factTextField.fact
    property real   textFieldPreferredWidth: -1
    property alias  textFieldUnitsLabel:     _factTextField.unitsLabel
    property alias  textFieldShowUnits:      _factTextField.showUnits
    property alias  textFieldShowHelp:       _factTextField.showHelp
    property alias  textField:               _factTextField
    property bool   vertical:                false  ///< true: label above the text field

    columns:        vertical ? 1 : 2
    columnSpacing:  ScreenTools.defaultFontPixelWidth * 2
    rowSpacing:     0

    QGCLabel {
        Layout.fillWidth:    !vertical
        Layout.minimumWidth: implicitWidth
        text:                label
        visible:             label !== ""
    }

    FactTextField {
        id:                     _factTextField
        Layout.preferredWidth:  textFieldPreferredWidth
    }
}

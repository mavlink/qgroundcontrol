/****************************************************************************
 *
 * (c) 2009-2022 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.15
import QtQuick.Layouts  1.15

import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0

RowLayout {
    property alias label:                   _label.text
    property alias fact:                    _factTextField.fact
    property real  textFieldPreferredWidth: -1
    property alias textFieldUnitsLabel:     _factTextField.unitsLabel
    property alias textFieldShowUnits:      _factTextField.showUnits
    property alias textFieldShowHelp:       _factTextField.showHelp

    spacing: ScreenTools.defaultFontPixelWidth * 2

    QGCLabel {
        id:                 _label  
        Layout.fillWidth:   true
    }

    FactTextField {
        id:                     _factTextField
        Layout.preferredWidth:  textFieldPreferredWidth
    }
}


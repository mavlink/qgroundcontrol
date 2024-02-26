/****************************************************************************
 *
 * (c) 2009-2022 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl.Controls
import QGroundControl.ScreenTools
import QGroundControl.FactSystem
import QGroundControl.FactControls

RowLayout {
    property alias label:                   label.text
    property alias fact:                    _comboBox.fact
    property alias indexModel:              _comboBox.indexModel
    property var   comboBox:                _comboBox
    property real  comboBoxPreferredWidth:  -1

    spacing: ScreenTools.defaultFontPixelWidth * 2

    signal activated(int index)

    QGCLabel {
        id:                 label  
        Layout.fillWidth:   true
    }

    FactComboBox {
        id:                     _comboBox
        Layout.preferredWidth:  comboBoxPreferredWidth
        sizeToContents:         true
        
        onActivated: (index) => { parent.activated(index) }
    }
}


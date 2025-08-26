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

import QGroundControl
import QGroundControl.Controls


import QGroundControl.FactControls

RowLayout {
    property string label:                   fact.shortDescription
    property alias  fact:                    _factTextArea.fact
    property real   textAreaPreferredWidth:  -1
    property alias  textAreaUnitsLabel:      _factTextArea.unitsLabel
    property alias  textAreaShowUnits:       _factTextArea.showUnits
    property alias  textAreaShowHelp:        _factTextArea.showHelp
    property alias  textArea:                _factTextArea

    spacing: ScreenTools.defaultFontPixelWidth * 2

    QGCLabel {
        Layout.fillWidth:   true
        text:               label
    }

    FactTextArea {
        id:                     _factTextArea
        Layout.preferredWidth:  textAreaPreferredWidth
    }
}


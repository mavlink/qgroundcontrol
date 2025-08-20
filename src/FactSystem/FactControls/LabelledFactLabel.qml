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
    property alias  fact:                    _factLabel.fact
    property real   labelPreferredWidth: -1
    property alias  factLabelShowUnits:      _factLabel.showUnits
    property alias  factLabel:               _factLabel

    spacing: ScreenTools.defaultFontPixelWidth * 2

    QGCLabel {
        Layout.fillWidth:   true
        text:               label
    }

    FactLabel {
        id:                     _factLabel
        Layout.preferredWidth:  labelPreferredWidth
    }
}


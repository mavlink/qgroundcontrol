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
    property alias fact:                    factSlider.fact
    property alias from:                    factSlider.from
    property alias to:                      factSlider.to
    property real  sliderPreferredWidth:    -1

    spacing: ScreenTools.defaultFontPixelWidth * 2

    QGCLabel {
        id:                 label  
        Layout.fillWidth:   true
    }

    FactSlider {
        id:                     factSlider
        Layout.preferredWidth:  sliderPreferredWidth
    }
}


/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls

import QGroundControl.Palette
import QGroundControl.ScreenTools

GroupBox {
    id: control

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    background: Rectangle {
        y:      control.topPadding - control.padding
        width:  parent.width
        height: parent.height - control.topPadding + control.padding
        color:  Qt.rgba(1, 1, 1, 0.025)
        radius: Math.round(ScreenTools.defaultFontPixelWidth * 0.75)
        border.color: Qt.rgba(0.82, 0.88, 0.94, 0.10)
        border.width: 1
    }

    label: QGCLabel {
        width:  control.availableWidth
        text:   control.title
    }
}

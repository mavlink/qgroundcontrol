/****************************************************************************
 *
 * (c) 2009-2026 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl.Controls
import QGroundControl.Palette
import QGroundControl.ScreenTools

PreFlightCheckButton {
    id: root

    QGCPalette {
        id: qgcPal
        colorGroupEnabled: root.enabled
    }

    contentItem: ColumnLayout {
        spacing: Math.round(ScreenTools.defaultFontPixelHeight * 0.25)

        QGCLabel {
            Layout.fillWidth:    true
            text:                root.name
            font.bold:           true
            font.pointSize:      ScreenTools.defaultFontPointSize
            wrapMode:            Text.WordWrap
            horizontalAlignment: Text.AlignLeft
            color:               qgcPal.buttonText
        }

        QGCLabel {
            Layout.fillWidth:    true
            text:                root.manualText
            font.pointSize:      ScreenTools.smallFontPointSize
            wrapMode:            Text.WordWrap
            horizontalAlignment: Text.AlignLeft
            color:               qgcPal.buttonText
            opacity:             0.72
        }
    }
}

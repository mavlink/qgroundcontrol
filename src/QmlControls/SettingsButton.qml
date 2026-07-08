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
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.Palette
import QGroundControl.ScreenTools

Button {
    id:             control
    padding:        ScreenTools.defaultFontPixelWidth * 0.95
    hoverEnabled:   !ScreenTools.isMobile
    autoExclusive:  true
    icon.color:     textColor

    property color textColor: checked || pressed ? qgcPal.buttonHighlightText : qgcPal.buttonText
    property real  activeStripWidth: Math.max(2, ScreenTools.defaultFontPixelWidth * 0.28)

    QGCPalette {
        id:                 qgcPal
        colorGroupEnabled:  control.enabled
    }

    background: Rectangle {
        color:      checked || pressed ? Qt.rgba(0.18, 0.20, 0.22, 0.96)
                                      : (enabled && hovered ? Qt.rgba(1, 1, 1, 0.055) : "transparent")
        radius:     Math.round(ScreenTools.defaultFontPixelWidth * 0.65)

        Rectangle {
            anchors.left:           parent.left
            anchors.verticalCenter: parent.verticalCenter
            width:                  control.activeStripWidth
            height:                 parent.height * 0.58
            radius:                 width / 2
            color:                  qgcPal.primaryButton
            visible:                checked || pressed
        }
    }

    contentItem: RowLayout {
        spacing: ScreenTools.defaultFontPixelWidth

        QGCColoredImage {
            source: control.icon.source
            color:  control.icon.color
            width:  ScreenTools.defaultFontPixelHeight * 1.08
            height: ScreenTools.defaultFontPixelHeight * 1.08
        }

        QGCLabel {
            id:                     displayText
            Layout.fillWidth:       true
            text:                   control.text
            color:                  control.textColor
            font.pointSize:         ScreenTools.controlFontPointSize
            horizontalAlignment:    QGCLabel.AlignLeft
        }
    }
}

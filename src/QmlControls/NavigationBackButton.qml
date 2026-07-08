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
    text:           qsTr("Back")
    hoverEnabled:   !ScreenTools.isMobile
    leftPadding:    ScreenTools.defaultFontPixelWidth * 0.78
    rightPadding:   ScreenTools.defaultFontPixelWidth * 1.0
    topPadding:     0
    bottomPadding:  0

    property string iconSource: "/res/nav-back.svg"

    QGCPalette {
        id:                 qgcPal
        colorGroupEnabled:  control.enabled
    }

    background: Rectangle {
        radius: Math.round(ScreenTools.defaultFontPixelWidth * 0.55)
        color:  control.pressed ? Qt.rgba(0.135, 0.140, 0.150, 0.48) :
                (control.hovered ? Qt.rgba(1, 1, 1, 0.060) : "transparent")
        border.color: control.hovered || control.pressed ? Qt.rgba(0.82, 0.90, 0.95, 0.16) : "transparent"
        border.width: control.hovered || control.pressed ? 1 : 0
    }

    contentItem: RowLayout {
        spacing: ScreenTools.defaultFontPixelWidth * 0.40

        QGCColoredImage {
            Layout.alignment:   Qt.AlignVCenter
            source:             control.iconSource
            color:              qgcPal.buttonText
            width:              ScreenTools.defaultFontPixelHeight * 1.05
            height:             width
            sourceSize.width:   width
            sourceSize.height:  height
            fillMode:           Image.PreserveAspectFit
        }

        QGCLabel {
            Layout.alignment:       Qt.AlignVCenter
            text:                   control.text
            color:                  qgcPal.buttonText
            font.pointSize:         ScreenTools.controlFontPointSize
            verticalAlignment:      Text.AlignVCenter
            maximumLineCount:       1
            elide:                  Text.ElideRight
        }
    }
}

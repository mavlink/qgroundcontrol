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

import QGroundControl
import QGroundControl.Controls
import QGroundControl.Palette
import QGroundControl.ScreenTools

// Important Note: Toolbar buttons must manage their checked state manually in order to support
// view switch prevention. This means they can't be checkable or autoExclusive.

Button {
    id:                 button
    height:             Math.max(ScreenTools.toolbarHeight * 0.82, ScreenTools.defaultFontPixelHeight * 2.6)
    leftPadding:        _horizontalMargin
    rightPadding:       _horizontalMargin
    checkable:          false

    property bool logo: false

    property real _horizontalMargin: ScreenTools.defaultFontPixelWidth * 0.75

    onCheckedChanged: checkable = false

    background: Rectangle {
        anchors.fill:   parent
        radius:         Math.round(ScreenTools.defaultFontPixelWidth * 0.7)
        color:          button.checked ? qgcPal.buttonHighlight : (button.hovered ? qgcPal.toolStripHoverColor : "transparent")
        border.color:   QGroundControl.corePlugin.showTouchAreas ? "red" : (button.checked ? qgcPal.primaryButton : "transparent")
        border.width:   QGroundControl.corePlugin.showTouchAreas ? 3 : (button.checked ? 1 : 0)
    }

    contentItem: Row {
        spacing:                ScreenTools.defaultFontPixelWidth * 0.75
        anchors.verticalCenter: button.verticalCenter
        Image {
            id:                     _logo
            height:                 ScreenTools.defaultFontPixelHeight * 1.65
            width:                  button.logo ? height : height
            sourceSize.height:      height
            fillMode:               Image.PreserveAspectFit
            source:                 button.icon.source
            mipmap:                 true
            visible:                button.logo
            anchors.verticalCenter: parent.verticalCenter
        }

        QGCColoredImage {
            id:                     _icon
            height:                 ScreenTools.defaultFontPixelHeight * 1.45
            width:                  height
            sourceSize.height:      parent.height
            fillMode:               Image.PreserveAspectFit
            color:                  button.checked ? qgcPal.buttonHighlightText : qgcPal.buttonText
            source:                 button.icon.source
            visible:                !button.logo
            anchors.verticalCenter: parent.verticalCenter
        }
        Label {
            id:                     _label
            visible:                text !== ""
            text:                   button.text
            color:                  button.checked ? qgcPal.buttonHighlightText : qgcPal.buttonText
            font.family:            ScreenTools.normalFontFamily
            font.pointSize:         ScreenTools.controlFontPointSize
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}

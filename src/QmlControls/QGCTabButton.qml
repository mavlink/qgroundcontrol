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

import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0

TabButton {
    id:             control
    topPadding:     _verticalPadding
    bottomPadding:  _verticalPadding
    leftPadding:    _horizontalPadding
    rightPadding:   _horizontalPadding
    focusPolicy:    Qt.ClickFocus
    //implicitWidth:  Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    //implicitHeight:     Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    property bool   primary:        false                               ///< primary button for a group of buttons
    property real   pointSize:      ScreenTools.defaultFontPointSize    ///< Point size for button text
    property bool   showBorder:     qgcPal.globalTheme === QGCPalette.Light
    property real   backRadius:     ScreenTools.buttonBorderRadius
    property real   heightFactor:   0.5

    property bool   _showHighlight:     enabled && (pressed | checked)
    property int    _horizontalPadding: ScreenTools.defaultFontPixelWidth
    property int    _verticalPadding:   Math.round(ScreenTools.defaultFontPixelHeight * heightFactor)
    property bool   _showIcon:          control.icon.source != ""

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    background: Rectangle {
        id:             backRect
        implicitWidth:  ScreenTools.implicitButtonWidth
        implicitHeight: ScreenTools.implicitButtonHeight
        //radius:         backRadius
        border.width:   showBorder ? 1 : 0
        border.color:   qgcPal.buttonBorder
        color:          _showHighlight ? qgcPal.buttonHighlight : qgcPal.button
    }

    contentItem: Item {
        implicitWidth:  _showIcon ? icon.width : text.implicitWidth
        implicitHeight: _showIcon ? icon.height : text.implicitHeight
        baselineOffset: text.y + text.baselineOffset

        QGCColoredImage {
            id:                     icon
            anchors.centerIn:       parent
            source:                 control.icon.source
            height:                 source === "" ? 0 : ScreenTools.defaultFontPixelHeight
            width:                  height
            color:                  _showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText
            fillMode:               Image.PreserveAspectFit
            sourceSize.height:      height
            visible:                _showIcon
        }

        Text {
            id:                     text
            anchors.centerIn:       parent
            antialiasing:           true
            text:                   control.text
            font.pointSize:         control.pointSize
            font.family:            ScreenTools.normalFontFamily
            color:                  _showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText
            visible:                !_showIcon
        }
    }
}

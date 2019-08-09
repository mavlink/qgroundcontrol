/****************************************************************************
 *
 *   (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                      2.11
import QtQuick.Controls             2.4

import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Button {
    id:                             button
    height:                         ScreenTools.defaultFontPixelHeight * 2
    width:                          height
    autoExclusive:                  true
    property bool showArrow:        true
    background: Rectangle {
        anchors.fill:               parent
        color:                      (button.checked || button.pressed) ? qgcPal.buttonHighlight : (button.flat ? Qt.rgba(0,0,0,0) : qgcPal.button)
        QGCColoredImage {
            height:                 parent.height * 0.25
            width:                  height
            sourceSize.height:      height
            fillMode:               Image.PreserveAspectFit
            color:                  qgcPal.buttonHighlight
            source:                 "/auterion/img/button_arrow_left.svg"
            visible:                button.checked && showArrow
            anchors.right:          parent.left
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    contentItem: QGCColoredImage {
        id:                         _icon
        height:                     ScreenTools.defaultFontPixelHeight * 0.5
        width:                      height
        sourceSize.height:          height
        fillMode:                   Image.PreserveAspectFit
        color:                      (button.checked || button.pressed) ? qgcPal.buttonHighlightText : qgcPal.buttonText
        source:                     button.icon.source
        anchors.centerIn:           parent
    }
}

/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.3
import QtQuick.Controls 2.4

import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Button {
    id:                 button
    height:             ScreenTools.defaultFontPixelHeight * 3
    autoExclusive:      true

    property bool logo: false
    property int imageSize: ScreenTools.defaultFontPixelHeight * 2

    function adjustImageSize(height, width)
    {
        _icon.height += height
        _icon.width += width
        iconRow.anchors.leftMargin = Math.abs(width)
        button.width += Math.abs(width/2)
        button.height += Math.abs(height/2)
    }

    background: Rectangle {
        anchors.fill: parent
        color:  logo ? qgcPal.brandingPurple : (checked ? qgcPal.button : Qt.rgba(0,0,0,0))
    }

    contentItem: Row {
        id: iconRow
        spacing:                    ScreenTools.defaultFontPixelWidth
        anchors.left:               button.left
        anchors.leftMargin:         ScreenTools.defaultFontPixelWidth
        anchors.verticalCenter:     button.verticalCenter

        QGCColoredImage {
            id:                     _icon
            height:                 imageSize
            width:                  imageSize
            sourceSize.height:      parent.height
            fillMode:               Image.PreserveAspectFit
            color:                  logo ? "white" : (button.checked ? qgcPal.buttonHighlightText : qgcPal.buttonText)
            source:                 button.icon.source
            anchors.verticalCenter: parent.verticalCenter
        }
        Label {
            id:                     _label
            visible:                text !== ""
            text:                   button.text
            color:                  button.checked ? qgcPal.buttonHighlightText : qgcPal.buttonText
            anchors.verticalCenter: parent.verticalCenter
        }
    }

}

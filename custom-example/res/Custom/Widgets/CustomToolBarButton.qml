/****************************************************************************
 *
 * (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 * @file
 *   @author Gus Grubba <gus@auterion.com>
 */

import QtQuick                      2.11
import QtQuick.Controls             2.4

import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Button {
    id:                             button
    autoExclusive:                  true

    background: Rectangle {
        anchors.fill:               parent
        color:                      qgcPal.buttonHighlight
        visible:                    (mouseArea.pressed || button.checked)
    }

    contentItem: Row {
        spacing:                    ScreenTools.defaultFontPixelWidth
        anchors.left:               button.left
        anchors.leftMargin:         ScreenTools.defaultFontPixelWidth
        anchors.verticalCenter:     button.verticalCenter
        Item {
            height:                 ScreenTools.defaultFontPixelHeight * 3
            width:                  1
        }
        QGCColoredImage {
            id:                     _icon
            height:                 ScreenTools.defaultFontPixelHeight
            width:                  height
            sourceSize.height:      parent.height
            fillMode:               Image.PreserveAspectFit
            color:                  qgcPal.buttonText
            source:                 button.icon.source
            anchors.verticalCenter: parent.verticalCenter
        }
        Label {
            id:                     _label
            visible:                text !== ""
            text:                   button.text
            color:                  qgcPal.buttonText
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    // Process hover events
    MouseArea {
        id:                         mouseArea
        anchors.fill:               parent
        onClicked:                  button.clicked()
    }
}

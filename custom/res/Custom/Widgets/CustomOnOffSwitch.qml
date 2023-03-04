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

import QtQuick 2.3

import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0

Rectangle {
    id:     _root
    height: Math.round(ScreenTools.defaultFontPixelHeight * 2)
    width:  ScreenTools.defaultFontPixelWidth  * 10
    color:  qgcPal.button
    border.color: qgcPal.windowShade
    border.width: 0

    property bool checked: true

    signal  clicked

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    Rectangle {
        width:      parent.width  * 0.5
        height:     parent.height
        color:      checked ? qgcPal.button : qgcPal.buttonHighlight
        border.color: qgcPal.text
        border.width: 0
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        QGCLabel {
            text: qsTr("Off")
            anchors.centerIn: parent
            color:      qgcPal.text
        }
    }
    Rectangle {
        width:      parent.width  * 0.5
        height:     parent.height
        color:      checked ? qgcPal.buttonHighlight : qgcPal.button
        border.color: qgcPal.text
        border.width: 0
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        QGCLabel {
            text:       qsTr("On")
            color:      qgcPal.buttonHighlightText
            anchors.centerIn: parent
        }
    }
    MouseArea {
        anchors.fill:   parent
        onClicked: {
            checked = !checked
            _root.clicked()
        }
    }
}

/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.11
import QtQuick.Controls 2.4

import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0

Rectangle {
    id:             _root
    width:          ScreenTools.defaultFontPixelHeight * 1.5
    height:         width
    radius:         width / 2
    border.color:   indicatorColor
    color:          "transparent"
    opacity:        0.75

    property color indicatorColor: "white"

    signal clicked

    Rectangle {
        anchors.margins:            _root.height / 6
        anchors.top:                parent.top
        anchors.bottom:             parent.bottom
        anchors.horizontalCenter:   parent.horizontalCenter
        width:                      1
        color:                      indicatorColor
    }

    Rectangle {
        anchors.margins:            _root.height / 6
        anchors.left:               parent.left
        anchors.right:              parent.right
        anchors.verticalCenter:     parent.verticalCenter
        height:                     1
        color:                      indicatorColor
    }

    QGCMouseArea {
        fillItem:   parent
        onClicked:  _root.clicked()
    }
}

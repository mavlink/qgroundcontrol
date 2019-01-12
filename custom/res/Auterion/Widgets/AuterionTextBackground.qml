/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.11
import QtQuick.Controls 1.2

Row {
    spacing:                        0
    property real contentWidth:     0
    property real contentHeight:    0
    Image {
        source:                 "/auterion/img/label_left_edge.svg"
        height:                 contentHeight
        width:                  height * 0.5
        antialiasing:           true
        sourceSize.height:      height
        fillMode:               Image.PreserveAspectFit
    }
    Rectangle {
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        width:                  parent.contentWidth
        color:                  "#000000"
    }
    Image {
        source:                 "/auterion/img/label_right_edge.svg"
        height:                 contentHeight
        width:                  height * 0.5
        antialiasing:           true
        sourceSize.height:      height
        fillMode:               Image.PreserveAspectFit
    }
}

/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief Battery Level Indicator
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick 2.3
import QGroundControl.ScreenTools  1.0
import QGroundControl.Palette      1.0

Item {
    id:     _root
    height: size
    width:  size * 0.5
    property bool   horizontal:     false
    property real   batteryLevel:   0.0
    property real   size:           ScreenTools.defaultFontPixelHeight
    property color  _batteryColor:  batteryLevel > 0.0 ? Qt.hsva(0.333333 * batteryLevel, 1, 1, 1) : qgcPal.buttonText
    QGCPalette { id: qgcPal; colorGroupEnabled: true }
    Column {
        spacing: 0
        Rectangle {
            width:  _root.width  * 0.35
            height: _root.height * 0.125
            color:  qgcPal.buttonText
            antialiasing: true
            anchors.horizontalCenter: parent.horizontalCenter
        }
        Rectangle {
            width:  _root.width
            height: _root.height * 0.75
            radius: 2
            color:  Qt.rgba(0,0,0,0)
            antialiasing: true
            border.width:  1
            border.color:  qgcPal.buttonText
            anchors.horizontalCenter: parent.horizontalCenter
            Rectangle {
                width:  parent.width * 0.6
                height: parent.height * batteryLevel * 0.75
                color:  _batteryColor
                antialiasing: true
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 2
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }
    transform: Rotation {
        origin.x:   _root.width  / 2
        origin.y:   _root.height / 2
        angle:      _root.horizontal ? 90.0 : 0.0
    }
}

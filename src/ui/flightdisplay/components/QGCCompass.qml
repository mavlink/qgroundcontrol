/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief QGC Compass
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick 2.4

Item {
    id:    root
    width:  parent.width  * 0.25 > 150 ? parent.width  * 0.25 : 150
    height: width
    property real heading : 0
    Image {
        id: compass
        anchors.centerIn: parent
        source: "/qml/compass.svg"
        mipmap: true
        width: root.width
        fillMode: Image.PreserveAspectFit
        transform: Rotation {
            origin.x: compass.width  / 2
            origin.y: compass.height / 2
            angle: -heading
        }
    }
    Image {
        id: pointer
        anchors.bottom: compass.top
        anchors.horizontalCenter: root.horizontalCenter
        source: "/qml/compassNeedle.svg"
        smooth:   true
        width:    compass.width * 0.1
        fillMode: Image.PreserveAspectFit
    }
    Rectangle {
        anchors.centerIn: compass
        width:  40
        height: 25
        border.color: Qt.rgba(1,1,1,0.25)
        color: Qt.rgba(1,1,1,0.1)
        Text {
            text: heading.toFixed(0)
            font.weight: Font.DemiBold
            color: "white"
            anchors.centerIn: parent
        }
    }
}



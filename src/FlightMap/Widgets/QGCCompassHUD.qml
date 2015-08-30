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
 *   @brief QGC Compass HUD
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick 2.4
import QGroundControl.Controls 1.0
import QGroundControl.ScreenTools 1.0

Item {
    id:    root
    property real heading : 0
    Image {
        id: compass
        anchors.centerIn: parent
        source: "/qmlimages/compass.svg"
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
        source: "/qmlimages/compassNeedle.svg"
        smooth:   true
        width:    compass.width * 0.1
        fillMode: Image.PreserveAspectFit
    }
    Rectangle {
        anchors.centerIn: compass
        width:  ScreenTools.defaultFontPixelSize * (3.33)
        height: ScreenTools.defaultFontPixelSize * (2.08)
        border.color: Qt.rgba(1,1,1,0.15)
        color: Qt.rgba(0,0,0,0.25)
        QGCLabel {
            text: heading.toFixed(0)
            font.weight: Font.DemiBold
            color: "white"
            anchors.centerIn: parent
        }
    }
}

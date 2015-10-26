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
 *   @brief QGC Compass Widget
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick 2.4

import QGroundControl.Controls 1.0
import QGroundControl.ScreenTools 1.0

Item {
    id:     root
    height: size

    property alias  heading:    compass.heading
    property alias  active:     attitude.active
    property alias  rollAngle:  attitude.rollAngle
    property alias  pitchAngle: attitude.pitchAngle
    property real   size:       ScreenTools.defaultFontPixelSize * (10)

    Rectangle {
        id:                 instrumentPannel
        anchors.left:       parent.left
        anchors.bottom:     parent.bottom
        height:             root.size
        width:              instruments.width + 8
        radius:             root.size / 2
        color:              Qt.rgba(0,0,0,0.5)

        Row {
            id:                 instruments
            height:             parent.height
            spacing:            4
            anchors.horizontalCenter:   parent.horizontalCenter
            QGCAttitudeWidget {
                id:             attitude
                size:           parent.height * 0.9
                anchors.verticalCenter: parent.verticalCenter
            }
            QGCCompassWidget {
                id:             compass
                size:           parent.height * 0.9
                anchors.verticalCenter: parent.verticalCenter
            }
        }

    }
}

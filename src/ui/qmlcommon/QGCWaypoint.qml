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
 *   @brief QGC Waypoint Marker
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick 2.4
import QtLocation 5.3

MapQuickItem {
    id: marker
    property alias waypointID: number
    anchorPoint.x: image.width  / 2
    anchorPoint.y: image.height / 2
    sourceItem: Rectangle {
        id: image
        width:  24
        height: 24
        border.color: Qt.rgba(0,0,0,0.75)
        color: Qt.rgba(0,0,0,0.5)
        Text {
            id: number
            anchors.centerIn: parent
            font.pointSize: 10
            color: "white"
        }
    }
}

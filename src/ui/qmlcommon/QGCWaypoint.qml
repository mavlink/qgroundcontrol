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
    property int waypointID: 0
    anchorPoint.x: markerIcon.width  / 2
    anchorPoint.y: markerIcon.height / 2
    sourceItem: Rectangle {
        id: markerIcon
        width:  30
        height: 30
        color:  markerMouseArea.containsMouse ? (markerMouseArea.pressed ? Qt.rgba(0.69,0.2,0.68,0.25) : Qt.rgba(0.69,0.2,0.68,0.75)) : Qt.rgba(0,0,0,0.5)
        radius: 8
        border.color: Qt.rgba(0,0,0,0.75)
        Text {
            id: number
            anchors.centerIn: parent
            font.pixelSize: 11
            font.weight: Font.DemiBold
            color: "white"
            text: marker.waypointID
        }
        MouseArea  {
            id: markerMouseArea
            enabled: !map.readOnly
            anchors.fill:    parent
            hoverEnabled:    true
            drag.target:     marker
            preventStealing: true
            property int pressX : -1
            property int pressY : -1
            property int jitterThreshold : 4
            onPressed : {
                pressX = mouse.x;
                pressY = mouse.y;
                map.currentMarker = -1;
                for (var i = 0; i < map.markers.length; i++) {
                    if (marker === map.markers[i]) {
                        map.currentMarker = i;
                        break;
                    }
                }
            }
            onPositionChanged: {
                if (Math.abs(pressX - mouse.x ) < jitterThreshold && Math.abs(pressY - mouse.y) < jitterThreshold) {
                    map.updateMarker(marker.coordinate, marker.waypointID)
                }
            }
        }
    }
}

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
 *   @brief QGC Map Background
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick 2.4
import QtPositioning 5.3
import QtLocation 5.3

Rectangle {
    id: root
    property real latitude:     37.803784
    property real longitude :   -122.462276
    property real zoomLevel:    12
    property real heading:      0
    property bool alwaysNorth:  true
    property alias mapItem:     map

    anchors.fill: parent
    color: Qt.rgba(0,0,0,0)
    clip: true

    function adjustSize() {
        if(root.visible) {
            if(alwaysNorth) {
                map.width  = root.width;
                map.height = root.height;
            } else {
                var diag = Math.ceil(Math.sqrt((root.width * root.width) + (root.height * root.height)));
                map.width  = diag;
                map.height = diag;
            }
        } else {
            map.width  = 1;
            map.height = 1;
        }
    }

    Plugin {
        id:   mapPlugin
        name: "osm"
    }

    Map {
        id: map
        plugin:     mapPlugin
        width:      1
        height:     1
        zoomLevel:  zoomLevel
        anchors.centerIn: parent
        center:     map.visible ? QtPositioning.coordinate(latitude, longitude) : QtPositioning.coordinate(0,0)
        transform: Rotation {
            origin.x: map.width  / 2
            origin.y: map.height / 2
            angle: map.visible ? (alwaysNorth ? 0 : -heading) : 0
        }
        gesture.flickDeceleration: 3000
        gesture.enabled: true
    }

    onVisibleChanged:       adjustSize();
    onWidthChanged:         adjustSize();
    onHeightChanged:        adjustSize();
    onAlwaysNorthChanged:   adjustSize();

}

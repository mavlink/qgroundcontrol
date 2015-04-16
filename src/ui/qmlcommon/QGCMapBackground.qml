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

import QGroundControl.Controls 1.0
import QGroundControl.FlightControls 1.0

Rectangle {
    id: root
    property real latitude:     37.803784
    property real longitude :   -122.462276
    property real zoomLevel:    18
    property real heading:      0
    property bool alwaysNorth:  true
    property bool interactive:  true
    property alias mapItem:     map

    color: Qt.rgba(0,0,0,0)
    clip: true

    function adjustSize() {
        if(root.visible) {
            if(true /*alwaysNorth*/) {
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

    function formatDistance(meters)
    {
        var dist = Math.round(meters)
        if (dist > 1000 ){
            if (dist > 100000){
                dist = Math.round(dist / 1000)
            }
            else{
                dist = Math.round(dist / 100)
                dist = dist / 10
            }
            dist = dist + " km"
        }
        else{
            dist = dist + " m"
        }
        return dist
    }

    Plugin {
        id:   mapPlugin
        name: "QGroundControl"
    }

    Map {
        id: map
        property real lon: (longitude >= -180 && longitude <= 180) ? longitude : 0
        property real lat: (latitude  >=  -90 && latitude  <=  90) ? latitude  : 0
        property variant scaleLengths: [5, 10, 25, 50, 100, 150, 250, 500, 1000, 2000, 5000, 10000, 20000, 50000, 100000, 200000, 500000, 1000000, 2000000]
        plugin:     mapPlugin
        width:      1
        height:     1
        zoomLevel:  root.zoomLevel
        anchors.centerIn: parent
        center:     QtPositioning.coordinate(lat, lon)
        /*
        // There is a bug currently in Qt where it fails to render a map taller than 6 tiles high. Until
        // that's fixed, we can't rotate the map as it would require a larger map, which can easely grow
        // larger than 6 tiles high.
        transform: Rotation {
            origin.x: map.width  / 2
            origin.y: map.height / 2
            angle: map.visible ? (alwaysNorth ? 0 : -heading) : 0
        }
        */
        gesture.flickDeceleration: 3000
        gesture.enabled: root.interactive

        onWidthChanged: {
            scaleTimer.restart()
        }

        onHeightChanged: {
            scaleTimer.restart()
        }

        onZoomLevelChanged:{
            scaleTimer.restart()
        }

        Component.onCompleted: {
            map.zoomLevel = 18
        }

        function calculateScale() {
            var coord1, coord2, dist, text, f
            f = 0
            coord1 = map.toCoordinate(Qt.point(0,scale.y))
            coord2 = map.toCoordinate(Qt.point(0+scaleImage.sourceSize.width,scale.y))
            dist = Math.round(coord1.distanceTo(coord2))
            if (dist === 0) {
                // not visible
            } else {
                for (var i = 0; i < scaleLengths.length-1; i++) {
                    if (dist < (scaleLengths[i] + scaleLengths[i+1]) / 2 ) {
                        f = scaleLengths[i] / dist
                        dist = scaleLengths[i]
                        break;
                    }
                }
                if (f === 0) {
                    f = dist / scaleLengths[i]
                    dist = scaleLengths[i]
                }
            }
            text = formatDistance(dist)
            scaleImage.width = (scaleImage.sourceSize.width * f) - 2 * scaleImageLeft.sourceSize.width
            scaleText.text = text
        }
    }

    QGCSlider {
        id: zoomSlider;
        minimum: map.minimumZoomLevel;
        maximum: map.maximumZoomLevel;
        opacity: 1
        visible: parent.visible
        z: map.z + 3
        anchors {
            bottom: parent.bottom;
            bottomMargin: 15; rightMargin: 20; leftMargin: 20
            left: parent.left
        }
        width: parent.width - anchors.rightMargin - anchors.leftMargin
        value: map.zoomLevel
        Binding {
            target: zoomSlider; property: "value"; value: map.zoomLevel
        }
        onValueChanged: {
            map.zoomLevel = value
        }
    }

    Item {
        id: scale
        parent: zoomSlider.parent
        visible: scaleText.text != "0 m"
        z: map.z + 2
        opacity: 1
        anchors {
            bottom: zoomSlider.top;
            bottomMargin: 8;
            left: zoomSlider.left
        }
        Image {
            id: scaleImageLeft
            source: "/qml/scale_end.png"
            anchors.bottom: parent.bottom
            anchors.left: parent.left
        }
        Image {
            id: scaleImage
            source: "/qml/scale.png"
            anchors.bottom: parent.bottom
            anchors.left: scaleImageLeft.right
        }
        Image {
            id: scaleImageRight
            source: "/qml/scale_end.png"
            anchors.bottom: parent.bottom
            anchors.left: scaleImage.right
        }
        QGCLabel {
            id: scaleText
            color: "white"
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
            anchors.bottom: parent.bottom
            anchors.left:   parent.left
            anchors.bottomMargin: 8
            text: "0 m"
        }
        Component.onCompleted: {
            map.calculateScale();
        }
    }

    Timer {
        id: scaleTimer
        interval: 100
        running:  false
        repeat:   false
        onTriggered: {
            map.calculateScale()
        }
    }

    onVisibleChanged: {
        adjustSize();
    }

    onAlwaysNorthChanged: {
        adjustSize();
    }

    onWidthChanged: {
        adjustSize();
    }

    onHeightChanged: {
        adjustSize();
    }
}

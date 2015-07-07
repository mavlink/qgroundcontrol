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
import QtQuick.Controls 1.3
import QtLocation 5.3
import QtPositioning 5.3

import QGroundControl.Controls 1.0
import QGroundControl.FlightControls 1.0
import QGroundControl.ScreenTools 1.0
import QGroundControl.MavManager 1.0

Item {
    id:     root
    clip:   true

    property real   latitude:           0
    property real   longitude:          0
    property real   zoomLevel:          18
    property real   heading:            0
    property bool   alwaysNorth:        true
    property bool   interactive:        true
    property bool   showWaypoints:      false
    property string mapName:            'defaultMap'
    property alias  mapItem:            map
    property alias  waypoints:          polyLine
    property alias  mapMenu:            mapTypeMenu
    property alias  readOnly:           map.readOnly

    Component.onCompleted: {
        map.zoomLevel   = 18
        map.markers     = []
        mapTypeMenu.update();
    }

    //-- Menu to select supported map types
    Menu {
        id: mapTypeMenu
        title: "Map Type..."
        enabled: root.visible
        ExclusiveGroup { id: currMapType }
        function setCurrentMap(mapID) {
            for (var i = 0; i < map.supportedMapTypes.length; i++) {
                if (mapID === map.supportedMapTypes[i].name) {
                    map.activeMapType = map.supportedMapTypes[i]
                    MavManager.saveSetting(root.mapName + "/currentMapType", mapID);
                    return;
                }
            }
        }
        function addMap(mapID, checked) {
            var mItem = mapTypeMenu.addItem(mapID);
            mItem.checkable = true
            mItem.checked   = checked
            mItem.exclusiveGroup = currMapType
            var menuSlot = function() {setCurrentMap(mapID);};
            mItem.triggered.connect(menuSlot);
        }
        function update() {
            clear()
            var mapID = ''
            if (map.supportedMapTypes.length > 0)
                mapID = map.activeMapType.name;
            mapID = MavManager.loadSetting(root.mapName + "/currentMapType", mapID);
            for (var i = 0; i < map.supportedMapTypes.length; i++) {
                var name = map.supportedMapTypes[i].name;
                addMap(name, mapID === name);
            }
            if(mapID != '')
                setCurrentMap(mapID);
        }
    }

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

    function updateWaypoints() {
        polyLine.path = []
        // Are we initialized?
        if (typeof map.markers != 'undefined' && typeof root.longitude != 'undefined') {
            map.deleteMarkers()
            if(root.showWaypoints) {
                for(var i = 0; i < MavManager.waypoints.length; i++) {
                    var coord = QtPositioning.coordinate(MavManager.waypoints[i].latitude, MavManager.waypoints[i].longitude, MavManager.waypoints[i].altitude);
                    polyLine.addCoordinate(coord);
                    map.addMarker(coord, MavManager.waypoints[i].id);
                }
                if (typeof MavManager.waypoints != 'undefined' && MavManager.waypoints.length > 0) {
                    root.longitude = MavManager.waypoints[0].longitude
                    root.latitude  = MavManager.waypoints[0].latitude
                }
                map.changed = false
            }
        }
    }

    Plugin {
        id:   mapPlugin
        name: "QGroundControl"
    }

    Connections {
        target: MavManager
        onWaypointsChanged: {
            root.updateWaypoints();
        }
    }

    onShowWaypointsChanged: {
        root.updateWaypoints();
    }

    Map {
        id: map

        property real   lon: (longitude >= -180 && longitude <= 180) ? longitude : 0
        property real   lat: (latitude  >=  -90 && latitude  <=  90) ? latitude  : 0
        property int    currentMarker
        property int    pressX : -1
        property int    pressY : -1
        property bool   changed:  false
        property bool   readOnly: false
        property variant scaleLengths: [5, 10, 25, 50, 100, 150, 250, 500, 1000, 2000, 5000, 10000, 20000, 50000, 100000, 200000, 500000, 1000000, 2000000]
        property variant markers

        plugin:     mapPlugin
        width:      1
        height:     1
        zoomLevel:  root.zoomLevel
        anchors.centerIn: parent
        center:     QtPositioning.coordinate(lat, lon)
        gesture.flickDeceleration: 3000
        gesture.enabled: root.interactive

        /*
        // There is a bug currently in Qt where it fails to render a map taller than 6 tiles high. Until
        // that's fixed, we can't rotate the map as it would require a larger map, which can easily grow
        // larger than 6 tiles high.
        // https://bugreports.qt.io/browse/QTBUG-45508
        transform: Rotation {
            origin.x: map.width  / 2
            origin.y: map.height / 2
            angle: map.visible ? (alwaysNorth ? 0 : -heading) : 0
        }
        */

        onWidthChanged: {
            scaleTimer.restart()
        }

        onHeightChanged: {
            scaleTimer.restart()
        }

        onZoomLevelChanged:{
            scaleTimer.restart()
        }

        MouseArea {
            anchors.fill: parent
            onDoubleClicked: {
                var coord = map.toCoordinate(Qt.point(mouse.x, mouse.y));
                map.addMarker(coord, polyLine.path.length);
                polyLine.addCoordinate(coord);
                map.changed = true;
            }
        }

        function updateMarker(coord, wpid)
        {
            if(wpid < polyLine.path.length) {
                var tmpPath = polyLine.path;
                tmpPath[wpid] = coord;
                polyLine.path = tmpPath;
                map.changed = true;
            }
        }

        function addMarker(coord, wpid)
        {
            var marker = Qt.createQmlObject ('QGCWaypoint {}', map)
            map.addMapItem(marker)
            marker.z = map.z + 1
            marker.coordinate = coord
            marker.waypointID = wpid
            // Update list of markers
            var count = map.markers.length
            var myArray = []
            for (var i = 0; i < count; i++){
                myArray.push(markers[i])
            }
            myArray.push(marker)
            markers = myArray
        }

        function deleteMarkers()
        {
            if (typeof map.markers != 'undefined') {
                var count = map.markers.length
                for (var i = 0; i < count; i++) {
                    map.markers[i].destroy()
                }
            }
            map.markers = []
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

        MapPolyline {
            id:             polyLine
            visible:        path.length > 1 && root.showWaypoints
            line.width:     3
            line.color:     map.changed ? "#f97a2e" : "#e35cd8"
            smooth:         true
            antialiasing:   true
        }
    }

    QGCSlider {
        id: zoomSlider;
        minimum: map.minimumZoomLevel;
        maximum: map.maximumZoomLevel;
        opacity: 1
        visible: parent.visible
        z: 1000
        anchors {
            bottom: parent.bottom;
            bottomMargin:   ScreenTools.defaultFontPixelSize * (1.25)
            rightMargin:    ScreenTools.defaultFontPixelSize * (1.66)
            leftMargin:     ScreenTools.defaultFontPixelSize * (1.66)
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
        visible: scaleText.text !== "0 m"
        z: map.z + 20
        opacity: 1
        anchors {
            bottom: zoomSlider.top;
            bottomMargin: ScreenTools.defaultFontPixelSize * (0.66);
            left: zoomSlider.left
            leftMargin: ScreenTools.defaultFontPixelSize * (0.33)
        }
        Image {
            id: scaleImageLeft
            source: "/qmlimages/scale_end.png"
            anchors.bottom: parent.bottom
            anchors.left: parent.left
        }
        Image {
            id: scaleImage
            source: "/qmlimages/scale.png"
            anchors.bottom: parent.bottom
            anchors.left: scaleImageLeft.right
        }
        Image {
            id: scaleImageRight
            source: "/qmlimages/scale_end.png"
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
            anchors.bottomMargin: ScreenTools.defaultFontPixelSize * (0.83)
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

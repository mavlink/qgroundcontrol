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

import QtQuick          2.4
import QtQuick.Controls 1.3
import QtLocation       5.3
import QtPositioning    5.3

import QGroundControl.Controls              1.0
import QGroundControl.FlightMap             1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Vehicle               1.0
import QGroundControl.Mavlink               1.0

Item {
    id:     root
    clip:   true

    property real   latitude:           0
    property real   longitude:          0
    property real   zoomLevel:          18
    property real   heading:            0
    property bool   interactive:        true
    property string mapName:            'defaultMap'
    property alias  mapItem:            map
    property alias  mapMenu:            mapTypeMenu
    property bool   showVehicles:       false
    property bool   showMissionItems:   false
    property bool   isSatelliteMap:     false

    Component.onCompleted: {
        map.zoomLevel   = 18
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
                    multiVehicleManager.saveSetting(root.mapName + "/currentMapType", mapID);
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
            mapID = multiVehicleManager.loadSetting(root.mapName + "/currentMapType", mapID);
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

/*
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
*/

    Plugin {
        id:   mapPlugin
        name: "QGroundControl"
    }

    Map {
        id: map

        property real   lon: (longitude >= -180 && longitude <= 180) ? longitude : 0
        property real   lat: (latitude  >=  -90 && latitude  <=  90) ? latitude  : 0
        property int    currentMarker
        property int    pressX : -1
        property int    pressY : -1
        property bool   changed:  false
        property variant scaleLengths: [5, 10, 25, 50, 100, 150, 250, 500, 1000, 2000, 5000, 10000, 20000, 50000, 100000, 200000, 500000, 1000000, 2000000]

        plugin:     mapPlugin
        width:      1
        height:     1
        zoomLevel:  root.zoomLevel
        anchors.centerIn: parent
        center:     QtPositioning.coordinate(lat, lon)
        gesture.flickDeceleration: 3000
        gesture.enabled: root.interactive

        // Add the vehicles to the map
        MapItemView {
            model: showVehicles ? multiVehicleManager.vehicles : 0
            
            delegate:
                VehicleMapItem {
                        vehicle:        object
                        isSatellite:    root.isSatelliteMap
                }
        }

        // Add the mission items to the map
        MapItemView {
            model: showMissionItems ? (multiVehicleManager.activeVehicle ? multiVehicleManager.activeVehicle.missionItems : 0) : 0
            
            delegate:
                MissionMapItem {
                    missionItem: object
                }
        }


/*
        onWidthChanged: {
            scaleTimer.restart()
        }

        onHeightChanged: {
            scaleTimer.restart()
        }

        onZoomLevelChanged:{
            scaleTimer.restart()
        }
*/

        MouseArea {
            anchors.fill: parent
            onDoubleClicked: {
                var coord = map.toCoordinate(Qt.point(mouse.x, mouse.y));
                map.addMarker(coord, polyLine.path.length);
                polyLine.addCoordinate(coord);
                map.changed = true;
            }
        }

/*
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
*/
    }
    
    // Vehicle GPS lock display
    Column {
        id:     gpsLockColumn
        y:      (parent.height - height) / 2
        width:  parent.width

        Repeater {
            model: multiVehicleManager.vehicles
            
            delegate:
                QGCLabel {
                    width:                  gpsLockColumn.width
                    horizontalAlignment:    Text.AlignHCenter
                    visible:                object.satelliteLock < 2
                    text:                   "No GPS Lock for Vehicle #" + object.id
                }
        }
    }
    
    // Mission item list
    ListView {
        id:                 missionItemSummaryList
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        anchors.left:       parent.left
        anchors.right:      controlWidgets.left
        anchors.bottom:     parent.bottom
        height:             ScreenTools.defaultFontPixelHeight * 7
        spacing:            ScreenTools.defaultFontPixelWidth / 2
        opacity:            0.75
        orientation:        ListView.Horizontal
        model:              multiVehicleManager.activeVehicle ? multiVehicleManager.activeVehicle.missionItems : 0

        property real _maxItemHeight: 0

        delegate:
            MissionItemSummary {
                opacity:        0.75
                missionItem:    object
            }
    }
    
    // This is used to determine the height of a horizontal scroll bar
    ScrollView {
        id:     scrollBarHeight
        x:      10000
        y:      10000
        width:  100
        height: 100
        
        Rectangle {
            height: 50
            width:  200
        }
    }

    /// Map control widgets
    Column {
        id:                 controlWidgets
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        anchors.right:      parent.right
        anchors.bottom:     parent.bottom
        spacing:            ScreenTools.defaultFontPixelWidth / 2

        QGCButton {
            id:     optionsButton
            text:   "Options"
            menu:   mapTypeMenu
        }
        
        Row {
            layoutDirection:    Qt.RightToLeft
            spacing:            ScreenTools.defaultFontPixelWidth / 2

            readonly property real _zoomIncrement: 1.0
            property real _buttonWidth: (optionsButton.width - spacing) / 2
            
            NumberAnimation {
                id: animateZoom
                
                property real startZoom
                property real endZoom
                
                target:     map
                properties: "zoomLevel"
                from:       startZoom
                to:         endZoom
                duration:   500
                
                easing {
                    type: Easing.OutExpo
                }
            }


            QGCButton {
                width:  parent._buttonWidth
                text:   "+"
                
                onClicked: {
                    var endZoomLevel = map.zoomLevel + parent._zoomIncrement
                    if (endZoomLevel > map.maximumZoomLevel) {
                        endZoomLevel = map.maximumZoomLevel
                    }
                    animateZoom.startZoom = map.zoomLevel
                    animateZoom.endZoom = endZoomLevel
                    animateZoom.start()
                }
            }
            
            QGCButton {
                width:  parent._buttonWidth
                text:   "-"
                
                onClicked: {
                    var endZoomLevel = map.zoomLevel - parent._zoomIncrement
                    if (endZoomLevel < map.minimumZoomLevel) {
                        endZoomLevel = map.minimumZoomLevel
                    }
                    animateZoom.startZoom = map.zoomLevel
                    animateZoom.endZoom = endZoomLevel
                    animateZoom.start()
                }
            }
        }
    }

/*

The slider and scale display are commented out for now to try to save real estate - DonLakeFlyer
Not sure if I'll bring them back or not. Need room for waypoint list at bottom

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
*/

    onVisibleChanged: {
        adjustSize();
    }

    onWidthChanged: {
        adjustSize();
    }

    onHeightChanged: {
        adjustSize();
    }
}

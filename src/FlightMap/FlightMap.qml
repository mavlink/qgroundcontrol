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

import QGroundControl.Controls              1.0
import QGroundControl.FlightMap             1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Vehicle               1.0

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

    Component.onCompleted: {
        map.zoomLevel   = 18
        mapTypeMenu.update();
        addExistingVehicles()
        updateMissionItemsConnections()
        updateMissionItems()
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

    // The following code is used to add and remove Vehicle markers from the map. Due to the following
    // problems this code must be here is the base FlightMap control:
    //      - If you pass a reference to the Map control into another object and then try to call
    //          functions such as addMapItem on it, it will fail telling you addMapItem is not a function
    //          on that object
    //      - Due to the fact that you need to dynamically add the MapQuickItems, they need to be able
    //          to reference the Vehicle they are associated with in some way. In order to do that
    //          we need to keep a separate array of Vehicles which must be at the top level of the object
    //          hierarchy in order for the dynamically added object to see it.

    property var _vehicles: []          ///< List of known vehicles
    property var _vehicleMapItems: []   ///< List of known vehicle map items

    Connections {
        target: multiVehicleManager

        onVehicleAdded:     addVehicle(vehicle)
        onVehicleRemoved:   removeVehicle(vehicle)
    }

    function addVehicle(vehicle) {
        if (!showVehicles) {
            return
        }
        
        var qmlItemTemplate = "VehicleMapItem { " +
                                    "coordinate:    _vehicles[%1].coordinate; " +
                                    "heading:       _vehicles[%1].heading " +
                                "}"

        var i = _vehicles.length
        qmlItemTemplate = qmlItemTemplate.replace("%1", i)
        qmlItemTemplate = qmlItemTemplate.replace("%1", i)

        _vehicles.push(vehicle)
        var mapItem = Qt.createQmlObject (qmlItemTemplate, map)
        _vehicleMapItems.push(mapItem)

        mapItem.z = map.z + 1
        map.addMapItem(mapItem)
    }

    function removeVehicle(vehicle) {
        if (!showVehicles) {
            return
        }
        
        for (var i=0; i<_vehicles.length; i++) {
            if (_vehicles[i] == vehicle) {
                _vehicles[i] = undefined
                map.removeMapItem(_vehicleMapItems[i])
                _vehicleMapItems[i] = undefined
                break
            }
        }
    }

    function addExistingVehicles() {
        if (!showVehicles) {
            return
        }
        
        for (var i=0; i<multiVehicleManager.vehicles.length; i++) {
            addVehicle(multiVehicleManager.vehicles[i])
        }
    }

    // The following code is used to show mission items on the FlightMap
    
    property var _missionItems: []      ///< List of known vehicles
    property var _missionMapItems: []   ///< List of known vehicle map items
    
    Connections {
        target: multiVehicleManager

        onActiveVehicleAvailableChanged: updateMissionItemsConnections()
    }
    
    function updateMissionItemsConnections() {
        if (multiVehicleManager.activeVehicleAvailable) {
            multiVehicleManager.activeVehicle.missionItemsChanged.connect(updateMissionItems)
        } else {
            // Previously active vehicle is about to go away, disconnect signals
            if (multiVehicleManager.activeVehicle) {
                multiVehicleManager.activeVehicle.missionItemsChanged.disconnect(updateMissionItems)
            }
        }
    }
    
    function addMissionItem(missionItem, index) {
        if (!showMissionItems) {
            console.warning("Shouldn't be called with showMissionItems=false")
            return
        }
        
        var qmlItemTemplate = "MissionMapItem { " +
                                    "coordinate:    _missionItems[%1].coordinate; " +
                                    "index:         %2" +
                                "}"
        
        var i = _missionItems.length
        qmlItemTemplate = qmlItemTemplate.replace("%1", i)
        qmlItemTemplate = qmlItemTemplate.replace("%2", index + 1)
        
        _missionItems.push(missionItem)
        var mapItem = Qt.createQmlObject (qmlItemTemplate, map)
        _missionMapItems.push(mapItem)
        
        mapItem.z = map.z + 1
        map.addMapItem(mapItem)
    }
    
    function removeMissionItem(missionItem) {
        if (!showMissionItems) {
            console.warning("Shouldn't be called with showMissionItems=false")
            return
        }
        
        for (var i=0; i<_missionItems.length; i++) {
            if (_missionItems[i] == missionItem) {
                // Qml has an annoying habit of not destroying remove Qml item until it hits the main loop.
                // Because of that we need to leave the the mission item references even though we have
                // removed the items, otherwise we'll get references to undefined errors until we hit the main
                // loop again.
                //_missionItems[i] = undefined
                map.removeMapItem(_missionMapItems[i])
                _missionMapItems[i] = undefined
                break
            }
        }
    }
    
    function updateMissionItems() {
        if (!showMissionItems) {
            return
        }
        
        var vehicle = multiVehicleManager.activeVehicle
        if (!vehicle) {
            console.warning("Why no active vehicle?")
            return
        }
        
        // Remove previous items
        for (var i=0; i<_missionItems.length; i++) {
            removeMissionItem(_missionItems[i])
        }
        _missionMapItems = []
        
        // Add new items
        for (var i=0; i<vehicle.missionItems.length; i++) {
            addMissionItem(vehicle.missionItems[i], i)
        }
    }
    
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
    
    /// Mission item list
    Row {
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        anchors.left:       parent.left
        anchors.right:      controlWidgets.left
        anchors.bottom:     parent.bottom
        spacing:            ScreenTools.defaultFontPixelWidth
        
        Repeater {
            model: multiVehicleManager.activeVehicle ? multiVehicleManager.activeVehicle.missionItems : 0
            
            MissionItemSummary {
                missionItem:        modelData
                missionItemIndex:   index + 1
            }
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

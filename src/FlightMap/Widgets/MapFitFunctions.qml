/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtPositioning    5.3

import QGroundControl           1.0
import QGroundControl.FlightMap 1.0

/// Set of functions for fitting the map view to a specific constraint
Item {
    property var    map
    property bool   usePlannedHomePosition      ///< true: planned home position used for calculations, false: vehicle home position use for calculations
    property var    planMasterController

    property var    _missionController:     planMasterController.missionController
    property var    _geoFenceController:    planMasterController.geoFenceController
    property var    _rallyPointController:  planMasterController.rallyPointController

    function fitHomePosition() {
        var homePosition = QtPositioning.coordinate()
        var activeVehicle = QGroundControl.multiVehicleManager.activeVehicle
        if (usePlannedHomePosition) {
            homePosition = _missionController.visualItems.get(0).coordinate
        } else if (activeVehicle) {
            homePosition = activeVehicle.homePosition
        }
        return homePosition
    }

    /// Normalize latitude to range: 0 to 180, S to N
    function normalizeLat(lat) {
        return lat + 90.0
    }

    /// Normalize longitude to range: 0 to 360, W to E
    function normalizeLon(lon) {
        return lon + 180.0
    }

    /// Fits the visible region of the map to inclues all of the specified coordinates. If no coordinates
    /// are specified the map will center to fitHomePosition()
    function fitMapViewportToAllCoordinates(coordList) {
        var mapFitViewport = Qt.rect(0, 0, map.width, map.height)
        if (coordList.length === 0) {
            var homeCoord = fitHomePosition()
            if (homeCoord.isValid) {
                map.center = homeCoord
            }
            return
        }

        // Create the normalized lat/lon corners for the coordinate bounding rect from the list of coordinates
        var north = normalizeLat(coordList[0].latitude)
        var south = north
        var east = normalizeLon(coordList[0].longitude)
        var west = east
        for (var i = 1; i < coordList.length; i++) {
            var lat = coordList[i].latitude
            var lon = coordList[i].longitude
            if (isNaN(lat) || lat == 0 || isNaN(lon) || lon == 0) {
                // Be careful of invalid coords which can happen when items are not yet complete
                continue
            }
            lat = normalizeLat(lat)
            lon = normalizeLon(lon)
            north = Math.max(north, lat)
            south = Math.min(south, lat)
            east  = Math.max(east,  lon)
            west  = Math.min(west,  lon)
        }

        // Expand the coordinate bounding rect to make room for the tools around the edge of the map
        var latDegreesPerPixel = (north - south) / mapFitViewport.height
        var lonDegreesPerPixel = (east  - west)  / mapFitViewport.width
        north = Math.min(north + (mapFitViewport.y * latDegreesPerPixel), 180)
        south = Math.max(south - ((map.height - mapFitViewport.bottom) * latDegreesPerPixel), 0)
        west  = Math.max(west  - (mapFitViewport.x * lonDegreesPerPixel), 0)
        east  = Math.min(east  + ((map.width - mapFitViewport.right) * lonDegreesPerPixel), 360)

        // Back off on zoom level
        east  = Math.min(east  * 1.0000075, 360)
        north = Math.min(north * 1.0000075, 180)
        west  = west  * 0.9999925
        south = south * 0.9999925

        // Fit the map region to the new bounding rect
        var topLeftCoord      = QtPositioning.coordinate(north - 90.0, west - 180.0)
        var bottomRightCoord  = QtPositioning.coordinate(south - 90.0, east - 180.0)
        map.setVisibleRegion(QtPositioning.rectangle(topLeftCoord, bottomRightCoord))
    }

    function addMissionItemCoordsForFit(coordList) {
        for (var i = 1; i < _missionController.visualItems.count; i++) {
            var missionItem = _missionController.visualItems.get(i)
            if (missionItem.specifiesCoordinate && !missionItem.isStandaloneCoordinate) {
                if(missionItem.boundingCube.isValid()) {
                    coordList.push(missionItem.boundingCube.pointNW)
                    coordList.push(missionItem.boundingCube.pointSE)
                } else {
                    coordList.push(missionItem.coordinate)
                }
            }
        }
    }

    function fitMapViewportToMissionItems() {
        if (!_missionController.visualItems) {
            // Being called prior to controller.start
            return
        }
        var coordList = [ ]
        addMissionItemCoordsForFit(coordList)
        fitMapViewportToAllCoordinates(coordList)
    }

    function addFenceItemCoordsForFit(coordList) {
        var i
        var homePosition = fitHomePosition()
        if (homePosition.isValid && _geoFenceController.circleEnabled) {
            var azimuthList = [ 0, 180, 90, 270 ]
            for (i = 0; i < azimuthList.length; i++) {
                var edgeCoordinate = homePosition.atDistanceAndAzimuth(_geoFenceController.circleRadius, azimuthList[i])
                coordList.push(edgeCoordinate)
            }
        }
        if (_geoFenceController.polygonEnabled && _geoFenceController.mapPolygon.path.count > 2) {
            for (i = 0; i < _geoFenceController.mapPolygon.path.count; i++) {
                coordList.push(_geoFenceController.mapPolygon.path[i])
            }
        }
    }

    function fitMapViewportToFenceItems() {
        var coordList = [ ]
        addFenceItemCoordsForFit(coordList)
        fitMapViewportToAllCoordinates(coordList)
    }

    function addRallyItemCoordsForFit(coordList) {
        for (var i=0; i<_rallyPointController.points.count; i++) {
            coordList.push(_rallyPointController.points.get(i).coordinate)
        }
    }

    function fitMapViewportToRallyItems() {
        var coordList = [ ]
        addRallyItemCoordsForFit(coordList)
        fitMapViewportToAllCoordinates(coordList)
    }

    function fitMapViewportToAllItems() {
        if (!_missionController.visualItems) {
            // Being called prior to controller.start
            return
        }
        var coordList = [ ]
        addMissionItemCoordsForFit(coordList)
        addFenceItemCoordsForFit(coordList)
        addRallyItemCoordsForFit(coordList)
        fitMapViewportToAllCoordinates(coordList)
    }
} // Item

/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtPositioning    5.3

import QGroundControl 1.0

/// Set of functions for fitting the map viewpoer to a specific constraint
Item {
    property var    map
    property bool   usePlannedHomePosition      ///< true: planned home position used for calculations, false: vehicle home position use for calculations
    property var    mapGeoFenceController
    property var    mapMissionController
    property var    mapRallyPointController

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    function fitHomePosition() {
        var homePosition = QtPositioning.coordinate()
        var activeVehicle = QGroundControl.multiVehicleManager.activeVehicle
        if (usePlannedHomePosition) {
            homePosition = mapMissionController.visualItems.get(0).coordinate
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
        return lon  + 180.0
    }

    /// Fits the visible region of the map to inclues all of the specified coordinates. If no coordinates
    /// are specified the map will center to fitHomePosition()
    function fitMapViewportToAllCoordinates(coordList) {
        var mapFitViewport = Qt.rect(0, 0, map.width, map.height)
        if (coordList.length == 0) {
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
        for (var i=1; i<coordList.length; i++) {
            var lat = normalizeLat(coordList[i].latitude)
            var lon = normalizeLon(coordList[i].longitude)

            north = Math.max(north, lat)
            south = Math.min(south, lat)
            east = Math.max(east, lon)
            west = Math.min(west, lon)
        }

        // Expand the coordinate bounding rect to make room for the tools around the edge of the map
        var latDegreesPerPixel = (north - south) / mapFitViewport.width
        var lonDegreesPerPixel = (east - west) / mapFitViewport.height
        north = Math.min(north + (mapFitViewport.y * latDegreesPerPixel), 180)
        south = Math.max(south - ((map.height - mapFitViewport.bottom) * latDegreesPerPixel), 0)
        west = Math.max(west - (mapFitViewport.x * lonDegreesPerPixel), 0)
        east = Math.min(east + ((map.width - mapFitViewport.right) * lonDegreesPerPixel), 360)

        // Fix the map region to the new bounding rect
        var topLeftCoord = QtPositioning.coordinate(north - 90.0, west - 180.0)
        var bottomRightCoord  = QtPositioning.coordinate(south - 90.0, east - 180.0)
        map.setVisibleRegion(QtPositioning.rectangle(topLeftCoord, bottomRightCoord))

        // Back off on zoom level
        map.zoomLevel = Math.abs(map.zoomLevel) - 1
    }

    function addMissionItemCoordsForFit(coordList) {
        var homePosition = fitHomePosition()
        if (homePosition.isValid) {
            coordList.push(homePosition)
        }
        for (var i=1; i<mapMissionController.visualItems.count; i++) {
            var missionItem = mapMissionController.visualItems.get(i)
            if (missionItem.specifiesCoordinate && !missionItem.isStandaloneCoordinate) {
                coordList.push(missionItem.coordinate)
            }
        }
    }

    function fitMapViewportToMissionItems() {
        if (!mapMissionController.visualItems) {
            // Being called prior to controller.start
            return
        }
        var coordList = [ ]
        addMissionItemCoordsForFit(coordList)
        fitMapViewportToAllCoordinates(coordList)
    }

    function addFenceItemCoordsForFit(coordList) {
        var homePosition = fitHomePosition()
        if (homePosition.isValid && mapGeoFenceController.circleEnabled) {
            var azimuthList = [ 0, 180, 90, 270 ]
            for (var i=0; i<azimuthList.length; i++) {
                var edgeCoordinate = homePosition.atDistanceAndAzimuth(mapGeoFenceController.circleRadius, azimuthList[i])
                coordList.push(edgeCoordinate)
            }
        }
        if (mapGeoFenceController.polygonEnabled && mapGeoFenceController.mapPolygon.path.count > 2) {
            for (var i=0; i<mapGeoFenceController.mapPolygon.path.count; i++) {
                coordList.push(mapGeoFenceController.mapPolygon.path[i])
            }
        }
    }

    function fitMapViewportToFenceItems() {
        var coordList = [ ]
        addFenceItemCoordsForFit(coordList)
        fitMapViewportToAllCoordinates(coordList)
    }

    function addRallyItemCoordsForFit(coordList) {
        for (var i=0; i<mapRallyPointController.points.count; i++) {
            coordList.push(mapRallyPointController.points.get(i).coordinate)
        }
    }

    function fitMapViewportToRallyItems() {
        var coordList = [ ]
        addRallyItemCoordsForFit(coordList)
        fitMapViewportToAllCoordinates(coordList)
    }

    function fitMapViewportToAllItems() {
        if (!mapMissionController.visualItems) {
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

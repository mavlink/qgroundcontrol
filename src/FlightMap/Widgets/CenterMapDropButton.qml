/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.2
import QtPositioning    5.3

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0

DropButton {
    id:                 dropButton
    dropDirection:      dropRight
    buttonImage:        "/qmlimages/MapCenter.svg"
    viewportMargins:    ScreenTools.defaultFontPixelWidth / 2
    lightBorders:       map.isSatelliteMap

    property var    map
    property rect   mapFitViewport
    property bool   usePlannedHomePosition      ///< true: planned home position used for calculations, false: vehicle home position use for calculations
    property var    geoFenceController
    property var    missionController
    property var    rallyPointController
    property bool   showMission:          true
    property bool   showAllItems:         true
    property bool   showFollowVehicle:    false
    property bool   followVehicle:        false

    function fitHomePosition() {
        var homePosition = QtPositioning.coordinate()
        var activeVehicle = QGroundControl.multiVehicleManager.activeVehicle
        if (usePlannedHomePosition) {
            homePosition = missionController.visualItems.get(0).coordinate
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
        if (coordList.length === 0) {
            map.center = fitHomePosition()
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
    }

    function addMissionItemCoordsForFit(coordList) {
        var homePosition = fitHomePosition()
        if (homePosition.isValid) {
            coordList.push(homePosition)
        }
        for (var i=1; i<missionController.visualItems.count; i++) {
            var missionItem = missionController.visualItems.get(i)
            if (missionItem.specifiesCoordinate && !missionItem.isStandaloneCoordinate) {
                coordList.push(missionItem.coordinate)
            }
        }
    }

    function fitMapViewportToMissionItems() {
        var coordList = [ ]
        addMissionItemCoordsForFit(coordList)
        fitMapViewportToAllCoordinates(coordList)
    }

    function addFenceItemCoordsForFit(coordList) {
        var i
        var homePosition = fitHomePosition()
        if (homePosition.isValid && geoFenceController.circleEnabled) {
            var azimuthList = [ 0, 180, 90, 270 ]
            for (i = 0; i < azimuthList.length; i++) {
                var edgeCoordinate = homePosition.atDistanceAndAzimuth(geoFenceController.circleRadius, azimuthList[i])
                coordList.push(edgeCoordinate)
            }
        }
        if (geoFenceController.polygonEnabled && geoFenceController.polygon.count() > 2) {
            for (i = 0; i < geoFenceController.polygon.count(); i++) {
                coordList.push(geoFenceController.polygon.path[i])
            }
        }
    }

    function fitMapViewportToFenceItems() {
        var coordList = [ ]
        addFenceItemCoordsForFit(coordList)
        fitMapViewportToAllCoordinates(coordList)
    }

    function addRallyItemCoordsForFit(coordList) {
        for (var i=0; i<rallyPointController.points.count; i++) {
            coordList.push(rallyPointController.points.get(i).coordinate)
        }
    }

    function fitMapViewportToRallyItems() {
        var coordList = [ ]
        addRallyItemCoordsForFit(coordList)
        fitMapViewportToAllCoordinates(coordList)
    }

    function fitMapViewportToAllItems() {
        var coordList = [ ]
        addMissionItemCoordsForFit(coordList)
        addFenceItemCoordsForFit(coordList)
        addRallyItemCoordsForFit(coordList)
        fitMapViewportToAllCoordinates(coordList)
    }

    dropDownComponent: Component {
        ColumnLayout {
            spacing: ScreenTools.defaultFontPixelWidth * 0.5

            QGCLabel { text: qsTr("Center map on:") }

            QGCButton {
                text:               qsTr("Mission")
                Layout.fillWidth:   true
                visible:            showMission
                enabled:            !followVehicleCheckBox.checked

                onClicked: {
                    dropButton.hideDropDown()
                    fitMapViewportToMissionItems()
                }
            }

            QGCButton {
                text:               qsTr("All items")
                Layout.fillWidth:   true
                visible:            showAllItems
                enabled:            !followVehicleCheckBox.checked

                onClicked: {
                    dropButton.hideDropDown()
                    fitMapViewportToAllItems()
                }
            }

            QGCButton {
                text:               qsTr("Launch")
                Layout.fillWidth:   true
                enabled:            !followVehicleCheckBox.checked

                onClicked: {
                    dropButton.hideDropDown()
                    map.center = fitHomePosition()
                }
            }

            QGCButton {
                text:               qsTr("Current Location")
                Layout.fillWidth:   true
                enabled:            map.gcsPosition ? map.gcsPosition.isValid && !followVehicleCheckBox.checked : false

                onClicked: {
                    dropButton.hideDropDown()
                    map.center = map.gcsPosition
                }
            }


            QGCButton {
                text:               qsTr("Specified Location")
                Layout.fillWidth:   true

                onClicked: {
                    dropButton.hideDropDown()
                    map.centerToSpecifiedLocation()
                }
            }

            QGCButton {
                text:               qsTr("Vehicle")
                Layout.fillWidth:   true
                enabled:            globals.activeVehicle && globals.activeVehicle.latitude != 0 && globals.activeVehicle.longitude != 0 && !followVehicleCheckBox.checked

                onClicked: {
                    dropButton.hideDropDown()
                    map.center = globals.activeVehicle.coordinate
                }
            }

            QGCCheckBox {
                id:         followVehicleCheckBox
                text:       qsTr("Follow Vehicle")
                checked:    followVehicle
                visible:    showFollowVehicle

                onClicked:  {
                    dropButton.hideDropDown()
                    dropButton.followVehicle = checked
                }
            }
        } // Column
    } // Component - dropDownComponent
} // DropButton

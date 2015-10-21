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

import QtQuick                  2.4
import QtQuick.Controls         1.3
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.2
import QtLocation               5.3
import QtPositioning            5.2

import QGroundControl               1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controllers   1.0

/// Flight Display View
Item {
    id: root

    property alias latitude: flightMap.latitude
    property alias longitude: flightMap.longitude

    // Top margin for all widgets. Used to prevent overlap with the toolbar
    property real   topMargin: 0

    // Used by parent to hide widgets when it displays something above in the z order.
    // Prevents z order drawing problems.
    property bool hideWidgets: false

    readonly property alias zOrderTopMost:   flightMap.zOrderTopMost
    readonly property alias zOrderWidgets:   flightMap.zOrderWidgets
    readonly property alias zOrderMapItems:  flightMap.zOrderMapItems

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    property var _activeVehicle: multiVehicleManager.activeVehicle

    readonly property var  _defaultVehicleCoordinate:   QtPositioning.coordinate(37.803784, -122.462276)
    readonly property real _defaultRoll:                0
    readonly property real _defaultPitch:               0
    readonly property real _defaultHeading:             0
    readonly property real _defaultAltitudeWGS84:       0
    readonly property real _defaultGroundSpeed:         0
    readonly property real _defaultAirSpeed:            0
    readonly property real _defaultClimbRate:           0

    readonly property string _mapName:              "FlightDisplayView"
    readonly property string _showMapBackgroundKey: "/showMapBackground"

    readonly property var _flightMap: flightMap

    property real _roll:            _activeVehicle ? (isNaN(_activeVehicle.roll) ? _defaultRoll : _activeVehicle.roll) : _defaultRoll
    property real _pitch:           _activeVehicle ? (isNaN(_activeVehicle.pitch) ? _defaultPitch : _activeVehicle.pitch) : _defaultPitch
    property real _heading:         _activeVehicle ? (isNaN(_activeVehicle.heading) ? _defaultHeading : _activeVehicle.heading) : _defaultHeading

    property var  _vehicleCoordinate:   _activeVehicle ? _activeVehicle.coordinate : _defaultVehicleCoordinate

    property real _altitudeWGS84:   _activeVehicle ? _activeVehicle.altitudeWGS84 : _defaultAltitudeWGS84
    property real _groundSpeed:     _activeVehicle ? _activeVehicle.groundSpeed : _defaultGroundSpeed
    property real _airSpeed:        _activeVehicle ? _activeVehicle.airSpeed : _defaultAirSpeed
    property real _climbRate:       _activeVehicle ? _activeVehicle.climbRate : _defaultClimbRate

    property bool _showMap: getBool(QGroundControl.flightMapSettings.loadMapSetting(flightMap.mapName, _showMapBackgroundKey, "1"))

    FlightDisplayViewController { id: _controller }
    MissionController { id: _missionController }

    ExclusiveGroup {
        id: _dropButtonsExclusiveGroup
    }

    // Validate _showMap setting
    Component.onCompleted: {
        delayLoader.source = "FlightDisplayViewDelayLoadOuter.qml"

        // We have to be careful to not reference root properties in a function which is in a subcomponent
        // until the root component has completed loading. Otherwise you get undefined references.
        flightMap.rootLoadCompleted = true
        flightMap.updateMapPosition(true /* force */)
        _setShowMap(_showMap)
    }

    function getBool(value) {
        return value === '0' ? false : true;
    }

    function setBool(value) {
        return value ? "1" : "0";
    }

    function _setShowMap(showMap) {
        _showMap = _controller.hasVideo ? showMap : true
        QGroundControl.flightMapSettings.saveMapSetting(flightMap.mapName, _showMapBackgroundKey, setBool(_showMap))
    }

    FlightMap {
        id:             flightMap
        anchors.fill:   parent
        mapName:        _mapName
        visible:        _showMap
        latitude:       root._defaultCoordinate.latitude
        longitude:      root._defaultCoordinate.longitude

        property var rootVehicleCoordinate: _vehicleCoordinate
        property bool rootLoadCompleted: false

        property bool _followVehicle: true

        onRootVehicleCoordinateChanged: updateMapPosition(false /* force */)

        Component.onCompleted: flightMapDelayLoader.source = "FlightDisplayViewDelayLoadInner.qml"

        function updateMapPosition(force) {
            if ((_followVehicle || force) && rootLoadCompleted) {
                flightMap.latitude = root._vehicleCoordinate.latitude
                flightMap.longitude = root._vehicleCoordinate.longitude
            }
        }
        // Home position
        MissionItemIndicator {
            label:          "H"
            coordinate:     (_activeVehicle && _activeVehicle.homePositionAvailable) ? _activeVehicle.homePosition : QtPositioning.coordinate(0, 0)
            visible:        _activeVehicle ? _activeVehicle.homePositionAvailable : false
            z:              flightMap.zOrderMapItems
        }

        // Add trajectory points to the map
        MapItemView {
            model: multiVehicleManager.activeVehicle ? multiVehicleManager.activeVehicle.trajectoryPoints : 0
            
            delegate:
                MapPolyline {
                    line.width: 3
                    line.color: "orange"
                    z:          flightMap.zOrderMapItems - 1


                    path: [
                        { latitude: object.coordinate1.latitude, longitude: object.coordinate1.longitude },
                        { latitude: object.coordinate2.latitude, longitude: object.coordinate2.longitude },
                    ]
                }
        }

        // Add the vehicles to the map
        MapItemView {
            model: multiVehicleManager.vehicles
            
            delegate:
                VehicleMapItem {
                        vehicle:        object
                        coordinate:     object.coordinate
                        isSatellite:    flightMap.isSatelliteMap
                        z:              flightMap.zOrderMapItems
                }
        }

        // Add the mission items to the map
        MissionItemView {
            model:          _missionController.missionItems
            zOrderMapItems: flightMap.zOrderMapItems
        }

        // Add lines between waypoints
        MissionLineView {
            model:          _missionController.waypointLines
            zOrderMapItems: flightMap.zOrderMapItems
        }

        Loader {
            id:             flightMapDelayLoader
            anchors.fill:   parent
        }

        // Used to make pinch zoom work
        MouseArea {
            anchors.fill: parent
        }
    } // Flight Map

    Loader {
        id:             delayLoader
        anchors.fill:   parent
    }
}

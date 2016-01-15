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

import QtQuick                      2.4
import QtQuick.Controls             1.3
import QtLocation                   5.3
import QtPositioning                5.2

import QGroundControl               1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controllers   1.0

FlightMap {
    id:             flightMap
    anchors.fill:   parent
    mapName:        _mapName

    property bool   _followVehicle:                 true
    property bool   _activeVehicleCoordinateValid:  multiVehicleManager.activeVehicle ? multiVehicleManager.activeVehicle.coordinateValid : false
    property var    activeVehicleCoordinate:        multiVehicleManager.activeVehicle ? multiVehicleManager.activeVehicle.coordinate : QtPositioning.coordinate()

    onActiveVehicleCoordinateChanged: {
        if (_followVehicle && _activeVehicleCoordinateValid && activeVehicleCoordinate.isValid) {
            _initialMapPositionSet = true
            flightMap.center  = activeVehicleCoordinate
        }
    }

    MissionController {
        id: _missionController
        Component.onCompleted: start(false /* editMode */)
    }

    // Add trajectory points to the map
    MapItemView {
        model: _mainIsMap ? multiVehicleManager.activeVehicle ? multiVehicleManager.activeVehicle.trajectoryPoints : 0 : 0
        delegate:
            MapPolyline {
                line.width: 3
                line.color: "orange"
                z:          QGroundControl.zOrderMapItems - 1
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
                    size:           _mainIsMap ? ScreenTools.defaultFontPixelHeight * 5 : ScreenTools.defaultFontPixelHeight * 2
                    z:              QGroundControl.zOrderMapItems
            }
    }

    // Add the mission items to the map
    MissionItemView {
        model: _mainIsMap ? _missionController.missionItems : 0
    }

    // Add lines between waypoints
    MissionLineView {
        model: _mainIsMap ? _missionController.waypointLines : 0
    }

    // Used to make pinch zoom work
    MouseArea {
        anchors.fill: parent
    }
}

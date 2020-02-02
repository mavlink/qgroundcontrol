/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtLocation       5.3
import QtPositioning    5.3

import QGroundControl           1.0
import QGroundControl.Controls  1.0
import QGroundControl.FlightMap 1.0

// Adds visual items associated with the Flight Plan to the map.
// Currently only used by Fly View even though it's called PlanMapItems!
Item {
    id: _root

    property var    map                 ///< Map control to show items on
    property bool   largeMapView        ///< true: map takes up entire view, false: map is in small window
    property var    masterController    ///< Reference to PlanMasterController for vehicle
    property var    vehicle             ///< Vehicle associated with these items

    property var    _map:                       map
    property var    _vehicle:                   vehicle
    property var    _missionController:         masterController.missionController
    property var    _geoFenceController:        masterController.geoFenceController
    property var    _rallyPointController:      masterController.rallyPointController
    property var    _missionLineViewComponent
    property bool   _isActiveVehicle:           vehicle.active

    property string fmode: vehicle.flightMode

    // Add the mission item visuals to the map
    Repeater {
        model: _isActiveVehicle && largeMapView ? _missionController.visualItems : 0

        delegate: MissionItemMapVisual {
            map:        _map
            vehicle:    _vehicle
            onClicked:  guidedActionsController.confirmAction(guidedActionsController.actionSetWaypoint, Math.max(object.sequenceNumber, 1))
        }
    }

    Component.onCompleted: {
        _missionLineViewComponent = missionLineViewComponent.createObject(map)
        if (_missionLineViewComponent.status === Component.Error)
            console.log(_missionLineViewComponent.errorString())
        map.addMapItem(_missionLineViewComponent)
    }

    Component.onDestruction: {
        _missionLineViewComponent.destroy()
    }

    Component {
        id: missionLineViewComponent

        MapPolyline {
            line.width: 3
            line.color: "#be781c"                           // Hack, can't get palette to work in here
            z:          QGroundControl.zOrderWaypointLines
            path:       _missionController.waypointPath
        }
    }
}

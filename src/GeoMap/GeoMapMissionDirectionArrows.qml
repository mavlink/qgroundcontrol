/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtPositioning

/// General flight-path direction indicators for the GeoMap: one arrow per
/// entry in MissionController.directionArrows (added to the second flight
/// path segment and every 5 segments thereafter, regardless of simple vs
/// complex item — see MissionController::_recalcFlightPathSegments),
/// mirroring the MapLineArrow Repeater in src/FlightMap/MapItems/PlanMapItems.qml.
Item {
    id: root

    property var missionController
    property var scene
    property var surfaceModel
    property real homeTerrainBias: 0   ///< See GeoMapWaypointItem.homeTerrainBias

    Repeater {
        model: root.missionController ? root.missionController.directionArrows : 0

        GeoMapSegmentArrow {
            scene: root.scene
            surfaceModel: root.surfaceModel
            fromCoord: QtPositioning.coordinate(object.coordinate1.latitude, object.coordinate1.longitude, object.coord1AMSLAlt + root.homeTerrainBias)
            toCoord: QtPositioning.coordinate(object.coordinate2.latitude, object.coordinate2.longitude, object.coord2AMSLAlt + root.homeTerrainBias)
            fraction: 0.75
        }
    }
}

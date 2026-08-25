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

import QGroundControl.Controls

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
            id: arrow
            scene: root.scene
            surfaceModel: root.surfaceModel
            fromCoord: {
                _terrainRev
                return _terrainLeg ? _drapedPoint(0.75 - _halfChordFraction)
                                   : QtPositioning.coordinate(object.coordinate1.latitude, object.coordinate1.longitude,
                                                              object.coord1AMSLAlt + root.homeTerrainBias)
            }
            toCoord: {
                _terrainRev
                return _terrainLeg ? _drapedPoint(0.75 + _halfChordFraction)
                                   : QtPositioning.coordinate(object.coordinate2.latitude, object.coordinate2.longitude,
                                                              object.coord2AMSLAlt + root.homeTerrainBias)
            }
            fraction: _terrainLeg ? 0.5 : 0.75

            // Terrain-frame legs are draped over the DEM (see
            // GeoMapMissionPath), so anchor the arrow to a short draped chord
            // around the 0.75 mark rather than the straight AMSL chord: the
            // chord midpoint puts the arrow on the ribbon and the chord ends
            // give it the local tangent heading
            readonly property bool _terrainLeg: !!root.surfaceModel && object.segmentType === FlightPathSegment.SegmentTypeTerrainFrame

            // Cap the chord at 50 m so it stays local to the ribbon on long
            // legs (a 10% chord averages away terrain curvature), while short
            // legs keep enough screen separation for a stable heading
            readonly property real _halfChordFraction: {
                const len = object.coordinate1.distanceTo(object.coordinate2)
                return len > 0 ? Math.min(0.05, 25 / len) : 0.05
            }

            // terrainHeightAt is not NOTIFY-reactive; read in the coordinate
            // bindings above so they re-sample as DEM patches stream in
            property int _terrainRev: 0

            // Same AGL-ramp draping as GeoMapMissionPath._appendTerrainLeg
            function _drapedPoint(f) {
                const c1 = object.coordinate1
                const c2 = object.coordinate2
                const point = c1.atDistanceAndAzimuth(c1.distanceTo(c2) * f, c1.azimuthTo(c2))
                const agl1 = object.coord1AMSLAlt - root.surfaceModel.terrainHeightAt(c1)
                const agl2 = object.coord2AMSLAlt - root.surfaceModel.terrainHeightAt(c2)
                return QtPositioning.coordinate(point.latitude, point.longitude,
                                                root.surfaceModel.terrainHeightAt(point) + agl1 + ((agl2 - agl1) * f) + root.homeTerrainBias)
            }

            Connections {
                target: root.surfaceModel
                enabled: arrow._terrainLeg
                function onTerrainHeightsChanged() { arrow._terrainRev++ }
            }
        }
    }
}

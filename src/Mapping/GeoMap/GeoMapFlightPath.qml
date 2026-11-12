/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick

import QGroundControl.GeoMap

/// Vehicle flight-path trail for the GeoMap: a red 3D ribbon in the scene at
/// true flight altitude, depth-tested so terrain occludes it. Width stays
/// constant in screen pixels via vertex-shader expansion. Stays visible in 2D
/// mode (crossfade3D off): the altitude offsets flatten with terrainScale so
/// it renders as the classic flat trail line. Altitudes get the same
/// DEM-vs-home-AMSL bias correction as GeoMapVehicleItem, applied at the
/// anchor so the whole trail shifts with it.
GeoMapTrack {
    id: root

    property var vehicle

    // DEM height minus vehicle-reported AMSL, sampled at home (same
    // correction as GeoMapVehicleItem so trail and marker stay aligned)
    function _updateHomeTerrainBias() {
        if (surfaceModel && vehicle && vehicle.homePosition.isValid && !isNaN(vehicle.homePosition.altitude)) {
            altitudeBias = surfaceModel.terrainDataHeightAt(vehicle.homePosition) - vehicle.homePosition.altitude
        } else {
            altitudeBias = 0
        }
    }

    // The trail flown so far, read when the geometry is created and when the vehicle changes: the points that
    // follow go to the geometry one by one
    path: _geometry && vehicle ? vehicle.trajectoryPoints.list() : []
    visible: !!vehicle

    onVehicleChanged: _updateHomeTerrainBias()
    onSurfaceModelChanged: _updateHomeTerrainBias()

    Connections {
        target: root._geometry && root.vehicle ? root.vehicle.trajectoryPoints : null

        function onPointAdded(coordinate) { root._geometry.appendPoint(coordinate) }
        function onUpdateLastPoint(coordinate) { root._geometry.updateLastPoint(coordinate) }
        function onPointsCleared() { root._geometry.clearPath() }
    }

    Connections {
        target: root.surfaceModel

        function onTerrainDataChanged() {
            root._updateHomeTerrainBias()
        }
    }

    Connections {
        target: root.vehicle

        function onHomePositionChanged() {
            root._updateHomeTerrainBias()
        }
    }
}

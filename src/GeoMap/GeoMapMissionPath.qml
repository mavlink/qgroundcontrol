/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick3D
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GeoMap

/// Mission path for the GeoMap: a 3D ribbon connecting every mission item in
/// sequence order (simple items at their coordinate; complex items via their
/// generic entryCoordinate/exitCoordinate, so the ribbon routes straight
/// through a complex item's own footprint rather than skipping it), at true
/// (AMSL) altitude. Stays visible in 2D mode (crossfade3D off), same as
/// GeoMapFlightPath. Rebuilt wholesale on any mission change (missions are
/// edited, not streamed, so the flight path's incremental append API is not
/// needed here).
GeoMapItem {
    id: root

    property var missionController
    property var vehicle       ///< Active vehicle, for the RTL-to-home segment
    // DEM-vs-home-AMSL bias (see GeoMapVehicleItem), computed once by the
    // consumer for the active vehicle and shared with GeoMapMissionItems
    // instead of being re-sampled here independently
    property real homeTerrainBias: 0
    property real lineWidth: ScreenTools.defaultFontPixelHeight / 5
    // Matches MissionLineView.qml's existing flight-path line color
    property color lineColor: QGroundControl.globalPalette.mapMissionTrajectory

    altitudeMode: GeoMapItem.Absolute
    crossfade3D: false
    coordinate: {
        if (!_geometry || !_geometry.anchorCoordinate.isValid) {
            return QtPositioning.coordinate()
        }
        const anchor = _geometry.anchorCoordinate
        return QtPositioning.coordinate(anchor.latitude, anchor.longitude, anchor.altitude + root.homeTerrainBias)
    }
    visible: !!missionController

    // The geometry lives inside the 3D delegate (created/destroyed with the
    // scene); this bridges its anchor out for the coordinate binding above
    property var _geometry: null

    readonly property var _camera: scene ? scene.camera : null

    // Pixel-to-scene-unit factor at unit distance; the shader scales it by
    // each vertex's own camera distance
    readonly property real _screenFactor: _camera ? _camera.unitsPerPixelAtUnitDistance : 0

    // Ordered coordinates (with AMSL altitude) of every item the vehicle
    // actually routes through, mirroring MissionController::_recalcFlightPathSegments:
    // items must specifiesCoordinate && !isStandaloneCoordinate to get a
    // segment (excludes RTL, which has no mission-encoded coordinate, and
    // standalone items like ROI); a qualifying item contributes its
    // entryCoordinate, plus its exitCoordinate too when that differs
    // (exitCoordinateSameAsEntry false) — for a simple item these are always
    // the same point, for a complex item this routes the ribbon straight
    // through its footprint edge-to-edge. Hitting RTL stops the walk and
    // links the last point to home instead (linkEndToHome). A land item ends
    // the walk at its entry: its interior path (loiter exit -> touchdown) is
    // drawn by the landing pattern visual from the slope start, not straight
    // from the loiter center, and _recalcFlightPathSegments draws no segment
    // after a landing item either. The walk starts at item 1 (item 0 is the
    // MissionSettingsItem, i.e. home); home is prepended only when the mission
    // starts from the ground — rover, or a takeoff command before the first
    // coordinate item — mirroring linkStartToHome. Recomputes automatically on
    // any structural or per-item change since the loop below reads every
    // NOTIFY-backed property it depends on.
    readonly property var _pathCoordinates: {
        const coordinates = []
        if (root.missionController) {
            const items = root.missionController.visualItems
            let linkStartToHome = root.vehicle ? root.vehicle.rover : false
            for (let i = 1; i < items.count; ++i) {
                const visualItem = items.get(i)
                if (visualItem.isSimpleItem && visualItem.command === MAVLinkEnums.MAV_CMD_NAV_RETURN_TO_LAUNCH) {
                    if (coordinates.length > 0 && root.vehicle && root.vehicle.homePosition.isValid) {
                        coordinates.push(root.vehicle.homePosition)
                    }
                    break
                }
                if (coordinates.length === 0 && visualItem.isTakeoffItem) {
                    linkStartToHome = true
                }
                if (visualItem.specifiesCoordinate && !visualItem.isStandaloneCoordinate) {
                    coordinates.push(QtPositioning.coordinate(visualItem.entryCoordinate.latitude,
                                                               visualItem.entryCoordinate.longitude,
                                                               visualItem.amslEntryAlt))
                    if (visualItem.isLandCommand) {
                        break
                    }
                    if (!visualItem.exitCoordinateSameAsEntry) {
                        coordinates.push(QtPositioning.coordinate(visualItem.exitCoordinate.latitude,
                                                                   visualItem.exitCoordinate.longitude,
                                                                   visualItem.amslExitAlt))
                    }
                }
            }
            if (linkStartToHome && coordinates.length > 0 && items.count > 0) {
                const home = items.get(0)
                if (home.coordinate.isValid) {
                    coordinates.unshift(QtPositioning.coordinate(home.coordinate.latitude,
                                                                  home.coordinate.longitude,
                                                                  home.amslEntryAlt))
                }
            }
        }
        return root._subdivided(coordinates)
    }

    // Legs longer than this are subdivided along the great circle (matching
    // MissionLineView.qml): the geometry joins points linearly in Web
    // Mercator, which diverges materially from the actual route over long
    // distances
    readonly property real _maxSegmentLengthM: 50000

    function _subdivided(coordinates) {
        const result = []
        for (let i = 0; i < coordinates.length; ++i) {
            const coord = coordinates[i]
            if (i > 0) {
                const prev = coordinates[i - 1]
                const distance = prev.distanceTo(coord)
                if (distance > root._maxSegmentLengthM) {
                    const segments = Math.ceil(distance / root._maxSegmentLengthM)
                    const azimuth = prev.azimuthTo(coord)
                    for (let j = 1; j < segments; ++j) {
                        const point = prev.atDistanceAndAzimuth((j * distance) / segments, azimuth)
                        result.push(QtPositioning.coordinate(point.latitude, point.longitude,
                                                             prev.altitude + ((coord.altitude - prev.altitude) * j) / segments))
                    }
                }
            }
            result.push(coord)
        }
        return result
    }

    function _reloadPath() {
        if (_geometry) {
            _geometry.setPath(root._pathCoordinates)
        }
    }

    on_PathCoordinatesChanged: _reloadPath()

    delegate3D: Component {
        // z offsets are relative altitudes; flatten them with the terrain
        // during the 2D<->3D transition (anchor z already tracks terrainScale
        // through GeoMapItem)
        Node {
            scale: Qt.vector3d(1, 1, root.scene ? root.scene.terrainScale : 1)

            Model {
                geometry: FlightPathGeometry {
                    id: pathGeometry

                    scene: root.scene
                }

                materials: CustomMaterial {
                    shadingMode: CustomMaterial.Unshaded
                    cullMode: Material.NoCulling
                    vertexShader: "shaders/flightpath.vert"
                    fragmentShader: "shaders/flightpath.frag"

                    property real lineWidth: root.lineWidth
                    property real screenFactor: root._screenFactor
                    property color pathColor: root.lineColor
                    // Ramps in as the terrain flattens, effectively disabling
                    // depth testing in 2D mode (see flightpath.vert)
                    property real depthPull: root.scene ? 0.5 * (1.0 - root.scene.terrainScale) : 0.0
                }

                Component.onCompleted: {
                    root._geometry = pathGeometry
                    root._reloadPath()
                }
                Component.onDestruction: root._geometry = null
            }
        }
    }
}

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
/// (AMSL) altitude. A leg is flown in its destination item's frame (ArduPilot
/// AC_WPNav): legs arriving at a terrain-frame item are draped over the DEM
/// with a linear AGL ramp, all other legs are straight lines. Straight legs
/// that pass below the terrain draw in collisionColor, as Plan view marks
/// them. Stays visible
/// in 2D mode (crossfade3D off), same as GeoMapFlightPath. Rebuilt wholesale
/// on any mission change (missions are edited, not streamed, so the flight
/// path's incremental append API is not needed here).
GeoMapItem {
    id: root

    enum Leg { Generic, Terrain, Takeoff, Land, Interior }

    property var missionController
    property var vehicle       ///< Active vehicle, for the RTL-to-home segment
    // DEM-vs-home-AMSL bias (see GeoMapVehicleItem), computed once by the
    // consumer for the active vehicle and shared with GeoMapMissionItems
    // instead of being re-sampled here independently
    property real homeTerrainBias: 0
    property real lineWidth: ScreenTools.defaultFontPixelHeight / 5
    // Matches MissionLineView.qml's existing flight-path line color
    property color lineColor: QGroundControl.globalPalette.mapMissionTrajectory
    property color collisionColor: "red"

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

    // Ordered route points ({coordinate, leg}) of every item the vehicle
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
    // coordinate item — mirroring linkStartToHome. leg is the type of the leg
    // ARRIVING at a point, mirroring segmentTypeForPair: Takeoff / Land
    // destinations first, then Terrain for a terrain-frame destination
    // (MAV_FRAME_GLOBAL_TERRAIN_ALT), else Generic; Interior is a complex
    // item's own entry -> exit span, which has no flight path segment. Recomputes automatically on any
    // structural or per-item change since the loop below reads every
    // NOTIFY-backed property it depends on.
    readonly property var _routePoints: {
        const points = []
        if (root.missionController) {
            const items = root.missionController.visualItems
            let linkStartToHome = root.vehicle ? root.vehicle.rover : false
            for (let i = 1; i < items.count; ++i) {
                const visualItem = items.get(i)
                if (visualItem.isSimpleItem && visualItem.command === MAVLinkEnums.MAV_CMD_NAV_RETURN_TO_LAUNCH) {
                    if (points.length > 0 && root.vehicle && root.vehicle.homePosition.isValid) {
                        points.push({ coordinate: root.vehicle.homePosition, leg: GeoMapMissionPath.Generic })
                    }
                    break
                }
                if (points.length === 0 && visualItem.isTakeoffItem) {
                    linkStartToHome = true
                }
                if (visualItem.specifiesCoordinate && !visualItem.isStandaloneCoordinate) {
                    points.push({
                        coordinate: QtPositioning.coordinate(visualItem.entryCoordinate.latitude,
                                                             visualItem.entryCoordinate.longitude,
                                                             visualItem.amslEntryAlt),
                        leg: visualItem.isTakeoffItem ? GeoMapMissionPath.Takeoff
                             : visualItem.isLandCommand ? GeoMapMissionPath.Land
                             : (visualItem.isSimpleItem && visualItem.altitudeFrame === QGroundControl.AltitudeFrameTerrain) ? GeoMapMissionPath.Terrain
                             : GeoMapMissionPath.Generic
                    })
                    if (visualItem.isLandCommand) {
                        break
                    }
                    if (!visualItem.exitCoordinateSameAsEntry) {
                        points.push({
                            coordinate: QtPositioning.coordinate(visualItem.exitCoordinate.latitude,
                                                                 visualItem.exitCoordinate.longitude,
                                                                 visualItem.amslExitAlt),
                            leg: GeoMapMissionPath.Interior
                        })
                    }
                }
            }
            if (linkStartToHome && points.length > 0 && items.count > 0) {
                const home = items.get(0)
                if (home.coordinate.isValid) {
                    points.unshift({
                        coordinate: QtPositioning.coordinate(home.coordinate.latitude,
                                                             home.coordinate.longitude,
                                                             home.amslEntryAlt),
                        leg: GeoMapMissionPath.Generic
                    })
                }
            }
        }
        return points
    }

    // Straight legs longer than this are subdivided along the great circle
    // (matching MissionLineView.qml): the geometry joins points linearly in
    // Web Mercator, which diverges materially from the actual route over long
    // distances
    readonly property real _maxSegmentLengthM: 50000

    // Terrain-frame legs sample the DEM at this spacing (same idea as
    // FenceWallGeometry), capped so a degenerate long leg can't explode the
    // vertex count
    readonly property real _terrainSampleSpacingM: 50
    readonly property int _maxTerrainSamplesPerLeg: 200

    // Terrain sampling can't live in the _routePoints binding: terrainHeightAt
    // is not NOTIFY-reactive, so draping is re-run explicitly here on route
    // changes and on terrainHeightsChanged as DEM patches stream in.
    // Returns the ribbon points and a highlight flag per point.
    function _expandedPath() {
        const points = root._routePoints
        const collisions = root._legCollisions
        const coordinates = []
        const highlights = []
        for (let i = 0; i < points.length; ++i) {
            const coord = points[i].coordinate
            if (i > 0) {
                const prev = points[i - 1].coordinate
                const collision = collisions[i]
                // Restart a recolored leg at a copy of its start point so the color
                // changes at the waypoint instead of blending along the leg
                if (collision !== highlights[highlights.length - 1]) {
                    coordinates.push(prev)
                    highlights.push(collision)
                }
                if (points[i].leg === GeoMapMissionPath.Terrain && root.surfaceModel) {
                    root._appendTerrainLeg(coordinates, prev, coord)
                } else {
                    coordinates.push(...root._longLegSubdivision(prev, coord))
                }
                coordinates.push(coord)
                while (highlights.length < coordinates.length) {
                    highlights.push(collision)
                }
            } else {
                coordinates.push(coord)
                highlights.push(points.length > 1 && collisions[1])
            }
        }
        return { coordinates: coordinates, highlights: highlights }
    }

    // Interior points of a straight leg (none when it is short enough)
    function _longLegSubdivision(prev, coord) {
        const result = []
        const distance = prev.distanceTo(coord)
        if (distance <= root._maxSegmentLengthM) {
            return result
        }
        const segments = Math.ceil(distance / root._maxSegmentLengthM)
        const azimuth = prev.azimuthTo(coord)
        for (let j = 1; j < segments; ++j) {
            const point = prev.atDistanceAndAzimuth((j * distance) / segments, azimuth)
            result.push(QtPositioning.coordinate(point.latitude, point.longitude,
                                                 prev.altitude + ((coord.altitude - prev.altitude) * j) / segments))
        }
        return result
    }

    // ArduPilot flies a terrain-frame leg at a height above terrain that
    // ramps from the origin's AGL to the destination's while riding the
    // terrain profile (AC_WPNav terrain offset); approximate that by draping
    // the leg over the DEM with a linear AGL ramp. AGLs are computed against
    // the same DEM the surface renders from, so the ribbon meets both
    // endpoint markers exactly (the DEM-vs-home AMSL bias cancels between
    // the endpoint and sample terms).
    function _appendTerrainLeg(result, prev, coord) {
        const distance = prev.distanceTo(coord)
        const steps = Math.min(Math.ceil(distance / root._terrainSampleSpacingM), root._maxTerrainSamplesPerLeg)
        if (steps < 2) {
            return
        }
        const azimuth = prev.azimuthTo(coord)
        const prevAgl = prev.altitude - root.surfaceModel.terrainHeightAt(prev)
        const destAgl = coord.altitude - root.surfaceModel.terrainHeightAt(coord)
        for (let j = 1; j < steps; ++j) {
            const s = j / steps
            const point = prev.atDistanceAndAzimuth(distance * s, azimuth)
            result.push(QtPositioning.coordinate(point.latitude, point.longitude,
                                                 root.surfaceModel.terrainHeightAt(point) + prevAgl + ((destAgl - prevAgl) * s)))
        }
    }

    // Per route point: the leg arriving there passes below the terrain
    // (FlightPathSegment::terrainCollision rules). Terrain-frame legs ride the
    // terrain and never collide.
    property var _legCollisions: []

    // FlightPathSegment::_collisionIgnoreMeters: takeoff and land legs meet the ground at one end
    readonly property real _collisionIgnoreMeters: 10

    function _computeLegCollisions() {
        const points = root._routePoints
        const collisions = points.map(() => false)
        // A rover follows the ground: its constant-altitude legs aren't a flight path
        if (!root.surfaceModel || (root.vehicle && root.vehicle.rover)) {
            return collisions
        }
        const straightUpTakeoff = !(root.vehicle && root.vehicle.fixedWing)
        for (let i = 1; i < points.length; ++i) {
            const leg = points[i].leg
            if (leg === GeoMapMissionPath.Terrain || leg === GeoMapMissionPath.Interior) {
                continue
            }
            const coord = points[i].coordinate
            let prev = points[i - 1].coordinate
            // MissionController::_createFlightPathSegmentWorker: a non-fixed-wing
            // climbs vertically, then flies over at the takeoff altitude
            if (leg === GeoMapMissionPath.Takeoff && straightUpTakeoff) {
                prev = QtPositioning.coordinate(prev.latitude, prev.longitude, coord.altitude)
            }
            // Check the same great-circle pieces _expandedPath draws
            const chain = [prev, ...root._longLegSubdivision(prev, coord), coord]
            for (let j = 1; j < chain.length && !collisions[i]; ++j) {
                collisions[i] = root.surfaceModel.segmentBelowTerrain(
                            root._demFrame(chain[j - 1]), root._demFrame(chain[j]),
                            (leg === GeoMapMissionPath.Takeoff && j === 1) ? root._collisionIgnoreMeters : 0,
                            (leg === GeoMapMissionPath.Land && j === chain.length - 1) ? root._collisionIgnoreMeters : 0)
            }
        }
        return collisions
    }

    // Route altitudes are vehicle AMSL; the rendered DEM sits homeTerrainBias above that frame
    function _demFrame(coord) {
        return QtPositioning.coordinate(coord.latitude, coord.longitude, coord.altitude + root.homeTerrainBias)
    }

    function _setGeometryPath() {
        const path = root._expandedPath()
        _geometry.setPath(path.coordinates, path.highlights)
    }

    function _reloadPath() {
        if (_geometry) {
            root._legCollisions = root._computeLegCollisions()
            root._setGeometryPath()
        }
    }

    function _refreshCollisions() {
        if (!_geometry) {
            return
        }
        const collisions = root._computeLegCollisions()
        const changed = collisions.some((collision, i) => collision !== root._legCollisions[i])
        root._legCollisions = collisions
        if (changed) {
            root._setGeometryPath()
        }
    }

    function _redrapePath() {
        if (_geometry) {
            root._setGeometryPath()
        }
    }

    // Qt.callLater coalesces rebuild bursts (per-patch terrainHeightsChanged
    // during tile streaming, route + terrain changes in one pass) into one
    // re-drape, same idea as FenceWallGeometry::_scheduleRebuild
    function _scheduleReloadPath() {
        Qt.callLater(root._reloadPath)
    }

    function _scheduleCollisionRefresh() {
        Qt.callLater(root._refreshCollisions)
    }

    on_RoutePointsChanged: _scheduleReloadPath()
    onHomeTerrainBiasChanged: _scheduleCollisionRefresh()

    readonly property bool _hasTerrainLeg: _routePoints.some(p => p.leg === GeoMapMissionPath.Terrain)

    Connections {
        target: root.surfaceModel

        // Draped legs follow the drawn mesh, which also changes on patch churn
        function onTerrainHeightsChanged() {
            if (root._hasTerrainLeg) {
                Qt.callLater(root._redrapePath)
            }
        }

        function onTerrainDataChanged() {
            root._scheduleCollisionRefresh()
        }
    }

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
                    property color highlightColor: root.collisionColor
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

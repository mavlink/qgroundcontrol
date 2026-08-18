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

/// Vehicle flight-path trail for the GeoMap: a red 3D ribbon in the scene at
/// true flight altitude, depth-tested so terrain occludes it. Width stays
/// constant in screen pixels via vertex-shader expansion. Stays visible in 2D
/// mode (crossfade3D off): the altitude offsets flatten with terrainScale so
/// it renders as the classic flat trail line. Altitudes get the same
/// DEM-vs-home-AMSL bias correction as GeoMapVehicleItem, applied at the
/// anchor so the whole trail shifts with it.
GeoMapItem {
    id: root

    property var vehicle
    property real lineWidth: ScreenTools.defaultFontPixelHeight / 5
    property color lineColor: "red"

    altitudeMode: GeoMapItem.Absolute
    crossfade3D: false
    coordinate: {
        if (!_geometry || !_geometry.anchorCoordinate.isValid) {
            return QtPositioning.coordinate()
        }
        const anchor = _geometry.anchorCoordinate
        return QtPositioning.coordinate(anchor.latitude, anchor.longitude, anchor.altitude + _homeTerrainBias)
    }
    visible: !!vehicle

    // The geometry lives inside the 3D delegate (created/destroyed with the
    // scene); this bridges its anchor out for the coordinate binding above
    property var _geometry: null

    readonly property var _camera: scene ? scene.camera : null

    // Pixel-to-scene-unit factor at unit distance; the shader scales it by
    // each vertex's own camera distance
    readonly property real _screenFactor: _camera ? _camera.unitsPerPixelAtUnitDistance : 0

    // DEM height minus vehicle-reported AMSL, sampled at home (same
    // correction as GeoMapVehicleItem so trail and marker stay aligned)
    property real _homeTerrainBias: 0

    function _updateHomeTerrainBias() {
        if (surfaceModel && vehicle && vehicle.homePosition.isValid && !isNaN(vehicle.homePosition.altitude)) {
            _homeTerrainBias = surfaceModel.terrainHeightAt(vehicle.homePosition) - vehicle.homePosition.altitude
        } else {
            _homeTerrainBias = 0
        }
    }

    function _reloadPath() {
        if (_geometry) {
            _geometry.setPath(vehicle ? vehicle.trajectoryPoints.list() : [])
        }
    }

    onVehicleChanged: {
        _updateHomeTerrainBias()
        _reloadPath()
    }
    onSurfaceModelChanged: _updateHomeTerrainBias()

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

            Connections {
                target: root.vehicle ? root.vehicle.trajectoryPoints : null

                function onPointAdded(coordinate) { pathGeometry.appendPoint(coordinate) }
                function onUpdateLastPoint(coordinate) { pathGeometry.updateLastPoint(coordinate) }
                function onPointsCleared() { pathGeometry.clearPath() }
            }
        }
    }

    Connections {
        target: root.surfaceModel

        function onTerrainHeightsChanged() {
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

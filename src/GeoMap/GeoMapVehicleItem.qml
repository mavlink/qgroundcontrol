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

/// Vehicle marker for the GeoMap: 2D top-down icon that crossfades into a
/// paper-airplane 3D model (heading/roll/pitch attitude) during the 2D<->3D
/// terrain-scale transition. Anchored at true AMSL altitude, bias-corrected
/// by the DEM-vs-home-AMSL offset so a landed vehicle sits exactly on the
/// rendered mesh despite DEM/GPS disagreement.
GeoMapItem {
    id: root

    property var vehicle
    property real size: ScreenTools.defaultFontPixelHeight * 3

    width: size
    height: size
    anchorPoint: Qt.point(width / 2, height / 2)
    altitudeMode: GeoMapItem.Absolute
    // coordinate.altitude (GLOBAL_POSITION_INT), not the altitudeAMSL fact:
    // on PX4 the fact switches to the baro-frame ALTITUDE message, which
    // disagrees with the GPS frame used by the trajectory and HOME_POSITION
    coordinate: vehicle ? QtPositioning.coordinate(vehicle.coordinate.latitude,
                                                   vehicle.coordinate.longitude,
                                                   vehicle.coordinate.altitude + _homeTerrainBias)
                        : QtPositioning.coordinate()
    visible: vehicle ? vehicle.coordinate.isValid : false

    readonly property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    readonly property real _dimming: vehicle && vehicle === _activeVehicle ? 1.0 : 0.5
    readonly property real _heading: vehicle && !isNaN(vehicle.heading.value) ? vehicle.heading.value : 0
    readonly property real _roll: vehicle && !isNaN(vehicle.roll.value) ? vehicle.roll.value : 0
    readonly property real _pitch: vehicle && !isNaN(vehicle.pitch.value) ? vehicle.pitch.value : 0
    readonly property var _camera: scene ? scene.camera : null

    // DEM height minus vehicle-reported AMSL, sampled at home. terrainHeightAt
    // is not a binding dependency, so re-sampled explicitly below as terrain
    // tiles arrive and when home moves.
    property real _homeTerrainBias: 0

    function _updateHomeTerrainBias() {
        if (surfaceModel && vehicle && vehicle.homePosition.isValid && !isNaN(vehicle.homePosition.altitude)) {
            _homeTerrainBias = surfaceModel.terrainHeightAt(vehicle.homePosition) - vehicle.homePosition.altitude
        } else {
            _homeTerrainBias = 0
        }
    }

    onVehicleChanged: _updateHomeTerrainBias()
    onSurfaceModelChanged: _updateHomeTerrainBias()

    // Constant apparent size: scene units per screen pixel at the camera's
    // look-at distance. Built-in QtQuick3D meshes are 100 units across.
    readonly property real _modelScale: _camera
        ? root.size * _camera.distance * _camera.unitsPerPixelAtUnitDistance / 100
        : 1

    Image {
        id: vehicleIcon

        anchors.fill: parent
        source: root.vehicle ? root.vehicle.vehicleImageOpaque : ""
        mipmap: true
        sourceSize.width: root.size
        fillMode: Image.PreserveAspectFit
        opacity: root.contentOpacity2D * root._dimming
        // Camera heading rotates map north on screen; the icon follows
        rotation: root._heading + (root._camera ? root._camera.heading : 0)
    }

    delegate3D: Component {
        // Paper-airplane dart matching the 2D icon. Scene axes are x east,
        // y north, z up; heading is CW from north while positive z-rotation
        // is CCW, hence the negation.
        Node {
            eulerRotation.z: -root._heading
            scale: Qt.vector3d(root._modelScale, root._modelScale, root._modelScale)

            Model {
                eulerRotation.x: root._pitch
                eulerRotation.y: root._roll
                geometry: PaperPlaneGeometry {}
                opacity: root._dimming

                materials: PrincipledMaterial {
                    vertexColorsEnabled: true
                    cullMode: Material.NoCulling
                }
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

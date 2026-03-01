pragma ComponentBehavior: Bound

import QtQuick
import QtQuick3D
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap

/// 3D terrain mesh overlay that syncs with the 2D map position/tilt/bearing.
/// Renders a semi-transparent altitude-colored terrain mesh on top of the map.
/// Uses a Loader to defer View3D creation until the map is fully initialized.
Item {
    id: root

    // Must be set by parent — the FlightMap to sync with
    required property var mapControl

    property bool   enabled:        true
    property real   heightScale:    1.0
    property real   terrainOpacity: 0.55
    property int    gridSize:       40
    readonly property bool terrainMeshSupported: _supportsQuick3D
    readonly property string terrainMeshDisableReason: {
        if (!terrainMeshSupported) {
            return qsTr("Terrain mesh disabled: unsupported graphics backend")
        }
        return ""
    }

    property bool _disableReasonLogged: false

    anchors.fill: parent

    // Only activate once the map has a valid center and the item has size
    readonly property bool _supportsQuick3D: {
        if (GraphicsInfo.api === GraphicsInfo.Software) {
            return false
        }
        if (GraphicsInfo.api === GraphicsInfo.Unknown) {
            return false
        }
        return true
    }

    readonly property bool _mapReady: enabled
                                      && _supportsQuick3D
                                      && mapControl !== null
                                      && mapControl.center !== undefined
                                      && !isNaN(mapControl.zoomLevel)
                                      && root.width > 0
                                      && root.height > 0

    function _logDisableReason() {
        if (!_disableReasonLogged && enabled && !terrainMeshSupported && terrainMeshDisableReason.length > 0) {
            console.warn(terrainMeshDisableReason)
            _disableReasonLogged = true
        }
    }

    Component.onCompleted: _logDisableReason()
    onEnabledChanged: _logDisableReason()
    onTerrainMeshSupportedChanged: _logDisableReason()

    Loader {
        id: overlayLoader
        anchors.fill: parent
        active: root._mapReady
        sourceComponent: terrainViewComponent
    }

    Component {
        id: terrainViewComponent

        Item {
            id: overlayItem
            anchors.fill: parent

            // Compute visible ground extent from zoom level
            // Ground resolution ≈ 156543.03 * cos(lat) / 2^zoom  (meters per pixel)
            readonly property real groundResolution: {
                if (!root.mapControl.center || !root.mapControl.center.isValid) {
                    return 100;
                }
                var latRad = root.mapControl.center.latitude * Math.PI / 180.0;
                var cosLat = Math.cos(latRad);
                if (cosLat <= 0) return 100;
                var res = 156543.03 * cosLat / Math.pow(2, root.mapControl.zoomLevel);
                return (isFinite(res) && res > 0) ? res : 100;
            }
            readonly property real visibleMeters: {
                var m = Math.max(overlayItem.width, overlayItem.height) * groundResolution * 1.5;
                return (isFinite(m) && m > 0) ? m : 5000;
            }

            readonly property real fovDeg:  45.0
            readonly property real camHeight: {
                var fovRad = fovDeg * Math.PI / 180.0;
                var h = (visibleMeters / 2.0) / Math.tan(fovRad / 2.0);
                return (isFinite(h) && h > 1) ? h : 5000;
            }

            readonly property real mapTilt:    root.mapControl.tilt || 0
            readonly property real mapBearing: root.mapControl.bearing || 0

            View3D {
                id: terrainView
                anchors.fill: parent
                renderMode: View3D.Offscreen
                camera: camera

                environment: SceneEnvironment {
                    backgroundMode:         SceneEnvironment.Transparent
                    clearColor:             "transparent"
                }

                // Camera rig: orbits around origin matching map tilt/bearing
                Node {
                    id: cameraRig
                    eulerRotation.y: -overlayItem.mapBearing

                    Node {
                        id: cameraTiltNode
                        // Map tilt 0 = looking straight down, 90 = horizon
                        eulerRotation.x: overlayItem.mapTilt

                        PerspectiveCamera {
                            id: camera
                            fieldOfView:    overlayItem.fovDeg
                            clipNear:       10
                            clipFar:        overlayItem.camHeight * 10
                            position:       Qt.vector3d(0, overlayItem.camHeight, 0)
                            eulerRotation.x: -90
                        }
                    }
                }

                DirectionalLight {
                    brightness:         0.8
                    eulerRotation.x:    -45
                    eulerRotation.y:    30
                    castsShadow:        false
                }

                DirectionalLight {
                    brightness:         0.3
                    eulerRotation.x:    -90
                }

                // Terrain mesh model
                Model {
                    id: terrainModel
                    visible: true

                    geometry: TerrainMeshData {
                        id: terrainMesh
                        center:         (root.mapControl.center && root.mapControl.center.isValid)
                                        ? root.mapControl.center
                                        : QtPositioning.coordinate(0, 0)
                        visibleMeters:  overlayItem.visibleMeters
                        gridSize:       root.gridSize
                        heightScale:    root.heightScale
                    }

                    materials: [
                        DefaultMaterial {
                            diffuseColor: "white"
                            vertexColorsEnabled: true
                            lighting: DefaultMaterial.NoLighting
                            opacity: root.terrainOpacity
                            cullMode: Material.NoCulling
                        }
                    ]
                }
            }
        }
    }
}

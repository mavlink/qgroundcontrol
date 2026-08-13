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
import QtQuick3D.Helpers
import QtPositioning

import QGroundControl
import QGroundControl.GeoMap

/// Experimental 2D/3D map control (preview feature): the GeoMap-engine
/// counterpart of FlightMap. Renders the LOD surface patch quadtree with full
/// camera gestures (pan, orbit, zoom, twist) and no view chrome.
Item {
    id: root

    property alias camera: geoCamera

    // Live SurfaceModel stats for debug overlays
    property alias patchCount: patchModel.patchCount
    property alias pendingCount: patchModel.pendingCount
    property alias maxZoomLevel: patchModel.maxZoomLevel
    property alias modelStats: patchModel.statsText
    property alias perfCapturing: patchModel.capturing

    function startPerfCapture() {
        patchModel.startCapture()
    }

    // Returns the CSV file path (empty on failure)
    function stopPerfCapture() {
        return patchModel.stopCapture()
    }

    readonly property int cameraAnimationMs: 500

    // Render-statistics overlay (FPS, frame timing, draw calls, texture/mesh
    // memory) for perf work; owned here because it needs the internal View3D
    property bool renderStats: false

    // Terrain displacement factor: 1 in 3D, 0 in 2D (map stays flat).
    // Startup mode is 2D, so terrain starts flattened.
    property real terrainScale: 0

    function analyzeSurface() {
        patchModel.analyzeSurface()
    }

    // Compass reset: animate heading back to north
    function animateHeadingToNorth() {
        headingAnimation.stop()
        // Wrap through 360 for the short way home; the setter normalizes the
        // final value back to 0
        headingAnimation.to = (geoCamera.heading > 180) ? 360 : 0
        headingAnimation.start()
    }

    // LOD checker colors for debug mode (imagery off)
    function _lodColor(centerX, centerY, span, zoomLevel) {
        const parity = (Math.round(centerX / span) + Math.round(centerY / span)) & 1
        return Qt.hsla((zoomLevel * 0.13) % 1.0, 0.5, parity ? 0.45 : 0.6, 1.0)
    }

    // Gestures jump a running mode transition to its end state rather
    // than stranding the pose mid-transition (2D mode locks tilt, so a
    // half-finished 3D->2D tilt could never recover by gesture)
    function completeCameraAnimations() {
        tiltAnimation.complete()
        headingAnimation.complete()
        terrainAnimation.complete()
    }

    GeoMapCamera {
        id: geoCamera
        objectName: "geoMapCamera"
        viewportSize: Qt.size(root.width, root.height)
        sceneOrigin: geoScene.sceneOrigin
        // Startup pose: overhead view of a land area with terrain relief so
        // imagery and elevation are visually verifiable (home-position
        // integration comes later). Until this center applies, the camera is
        // unpositioned and SurfaceModel builds nothing (see
        // GeoMapCamera::isPositioned), so the null-island default pose never
        // triggers doomed terrain fetches at (0,0).
        center: QtPositioning.coordinate(47.6329078, -122.0876875)
    }

    GeoScene {
        id: geoScene
        objectName: "geoMapScene"
        camera: geoCamera
    }

    SurfacePatchModel {
        id: patchModel
        objectName: "geoMapPatchModel"
        scene: geoScene
        terrain: true
        statsEnabled: root.renderStats
        // Drape the map imagery the rest of QGC uses (empty disables imagery)
        mapType: QGroundControl.settingsManager.flightMapSettings.mapProvider.rawValue
                 + " " + QGroundControl.settingsManager.flightMapSettings.mapType.rawValue
    }

    View3D {
        id: viewport
        objectName: "geoMapViewport"
        anchors.fill: parent

        environment: SceneEnvironment {
            clearColor: "#1a2028"
            backgroundMode: SceneEnvironment.Color
            // Distance haze toward the background color hides the far-field
            // LOD rings at the patch range edge
            fog: Fog {
                enabled: true
                color: "#1a2028"
                depthEnabled: true
                depthNear: 10 * geoCamera.distance
                depthFar: (patchModel.maxRangeMultiplier + 1) * geoCamera.distance
            }
        }

        DirectionalLight {
            eulerRotation.x: -60
            ambientColor: Qt.rgba(0.4, 0.4, 0.4, 1.0)
        }

        // Scene camera fully driven by the GeoMapCamera pose. Clip planes
        // scale with distance so the surface stays visible from close-up
        // through whole-earth views.
        PerspectiveCamera {
            id: sceneCamera
            objectName: "geoMapSceneCamera"
            position: geoCamera.scenePosition
            rotation: geoCamera.sceneRotation
            fieldOfView: geoCamera.fieldOfView
            clipNear: Math.max(1, geoCamera.distance / 1000)
            // Ray length to the farthest retained ground point is at most
            // maxRange + camera height <= (maxRangeMultiplier + 1) * distance
            clipFar: Math.max(100000, geoCamera.distance * (patchModel.maxRangeMultiplier + 1))
        }

        // One surface patch per active SurfaceModel entry: draped tile
        // imagery once delivered, LOD-colored fallback while loading.
        // The shared z-scale converts true-meter heights into mercator scene
        // units (verticalScale) and animates terrain flat<->full during the
        // 2D/3D mode transition (terrainScale) without geometry rebuilds:
        // one binding here instead of one per delegate (delegate creation
        // cost dominates patch churn, and transitions re-evaluated it N
        // times per frame). Never exactly 0: a singular scale breaks the
        // normal matrix.
        Node {
            scale: Qt.vector3d(1, 1, Math.max(0.0001, root.terrainScale * geoScene.verticalScale))

            Repeater3D {
                model: patchModel
                delegate: Model {
                    id: patchDelegate
                    objectName: "geoMapPatchDelegate"
                    required property real centerX
                    required property real centerY
                    required property real span
                    required property int zoomLevel
                    required property var heights
                    required property bool covered
                    required property var edgeLodDeltas
                    required property var tileImage
                    required property bool hasTileImage

                    // A covered pending patch stays hidden: its empty flat grid
                    // would z-fight the retained retiring cover
                    visible: !covered
                    position: Qt.vector3d(centerX, centerY, 0)
                    geometry: PatchGeometry {
                        gridSize: patchModel.gridSize
                        span: patchDelegate.span
                        heights: patchDelegate.heights
                        edgeLodDeltas: patchDelegate.edgeLodDeltas
                    }

                    Texture {
                        id: patchTexture
                        // Clamp: default Repeat wrap bleeds opposite-edge texels
                        // at UV 0/1, drawing 1-px seams on every patch border
                        tilingModeHorizontal: Texture.ClampToEdge
                        tilingModeVertical: Texture.ClampToEdge
                        textureData: PatchTextureData {
                            image: patchDelegate.tileImage
                        }
                    }

                    materials: [
                        DefaultMaterial {
                            cullMode: Material.NoCulling
                            // Unlit: tiles carry their own shading, and lighting
                            // would show the per-patch normal seams
                            lighting: DefaultMaterial.NoLighting
                            diffuseMap: patchDelegate.hasTileImage ? patchTexture : null
                            // Loading fallback: quiet neutral when imagery is on
                            // (LOD flash on zoom); LOD checker colors in debug mode
                            diffuseColor: patchDelegate.hasTileImage
                                          ? "white"
                                          : (patchModel.mapType !== ""
                                             ? "#3a4048"
                                             : root._lodColor(patchDelegate.centerX, patchDelegate.centerY,
                                                              patchDelegate.span, patchDelegate.zoomLevel))
                        }
                    ]
                }
            }
        }

        // Gestures (Viewer3D semantics): the ground point under the cursor
        // at gesture start stays under the cursor throughout. Any gesture
        // completes a running mode/compass animation to its end state so
        // the two never fight over the same pose properties.
        DragHandler {
            id: panHandler
            target: null
            acceptedButtons: Qt.LeftButton
            onActiveChanged: {
                if (active) {
                    root.completeCameraAnimations()
                    // Anchor at the current position, not pressPosition: after a
                    // two-finger pinch drops to one finger this handler re-activates
                    // with a stale pressPosition, and anchoring there jumps the map.
                    geoCamera.beginPan(centroid.position)
                }
            }
            onCentroidChanged: {
                if (active) {
                    geoCamera.panTo(centroid.position)
                }
            }
        }

        // Right-drag orbits: full width = 360 deg heading, full height = 180 deg tilt.
        // Mouse/touchpad only: acceptedButtons doesn't filter touch points, so
        // without acceptedDevices this handler steals single-finger drags from
        // panHandler on touchscreens (touch orbits via PinchHandler twist instead).
        // TouchPad keeps trackpad secondary-click drag working.
        DragHandler {
            id: orbitHandler
            target: null
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            acceptedButtons: Qt.RightButton
            onActiveChanged: {
                if (active) {
                    root.completeCameraAnimations()
                    geoCamera.beginOrbit(centroid.pressPosition)
                }
            }
            onCentroidChanged: {
                if (active) {
                    geoCamera.orbitTo(centroid.position)
                }
            }
        }

        WheelHandler {
            target: null
            onWheel: (event) => {
                root.completeCameraAnimations()
                geoCamera.zoom(event.angleDelta.y, point.position)
            }
        }

        // Pinch zooms about the centroid; two-finger twist rotates around it
        // (handler rotation is positive clockwise on screen; heading follows
        // so the world tracks the fingers)
        PinchHandler {
            id: pinchHandler
            target: null
            onActiveChanged: {
                if (active) {
                    root.completeCameraAnimations()
                }
            }
            onScaleChanged: (delta) => geoCamera.zoomBy(1 / delta, centroid.position)
            onRotationChanged: (delta) => geoCamera.rotateBy(delta, centroid.position)
        }
    }

    DebugView {
        objectName: "geoMapRenderStats"
        anchors.left: parent.left
        anchors.top: parent.top
        source: viewport
        visible: root.renderStats
        resourceDetailsVisible: true
    }

    NumberAnimation {
        id: tiltAnimation
        target: geoCamera
        property: "tilt"
        duration: root.cameraAnimationMs
        easing.type: Easing.InOutQuad
    }

    NumberAnimation {
        id: headingAnimation
        target: geoCamera
        property: "heading"
        duration: root.cameraAnimationMs
        easing.type: Easing.InOutQuad
    }

    NumberAnimation {
        id: terrainAnimation
        target: root
        property: "terrainScale"
        duration: root.cameraAnimationMs
        easing.type: Easing.InOutQuad
    }

    // Single transition path for every mode writer (button, goTo2D/3D,
    // C++): mode changes always animate tilt and terrain to the mode's
    // targets, so camera state and terrain displacement cannot desync
    Connections {
        target: geoCamera
        function onModeChanged() {
            const to3D = geoCamera.mode === GeoMapCamera.Mode3D
            tiltAnimation.stop()
            tiltAnimation.to = to3D ? geoCamera.default3DTilt : 0
            tiltAnimation.start()
            terrainAnimation.stop()
            terrainAnimation.to = to3D ? 1 : 0
            terrainAnimation.start()
        }
    }
}

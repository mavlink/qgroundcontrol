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
import QGroundControl.GeoView

/// Experimental 2D/3D map view (preview feature). Renders the LOD surface
/// patch quadtree with full camera gestures (pan, orbit, zoom, twist).
Item {
    id: root

    QGCPalette { id: qgcPal }

    Rectangle {
        id: toolbar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: ScreenTools.toolbarHeight
        color: qgcPal.toolbarBackground

        QGCToolBarButton {
            objectName: "toolbar_qgcLogo"
            height: parent.height
            icon.source: "/res/QGCLogoFull.svg"
            logo: true
            onClicked: mainWindow.showToolSelectDialog()
        }

        QGCLabel {
            anchors.centerIn: parent
            text: qsTr("GeoView (preview)")
            font.pointSize: ScreenTools.largeFontPointSize
        }
    }

    // The 3D scene only exists while GeoView is the active view: View3D
    // costs RHI/scene-graph resources even when hidden.
    Loader {
        id: sceneLoader
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: toolbar.bottom
        anchors.bottom: parent.bottom
        active: root.visible
        sourceComponent: Item {
            id: sceneRoot

            readonly property int cameraAnimationMs: 500

            // Terrain displacement factor: 1 in 3D, 0 in 2D (map stays flat).
            // Startup mode is 2D, so terrain starts flattened.
            property real terrainScale: 0

            // Gestures jump a running mode transition to its end state rather
            // than stranding the pose mid-transition (2D mode locks tilt, so a
            // half-finished 3D->2D tilt could never recover by gesture)
            function completeCameraAnimations() {
                tiltAnimation.complete()
                headingAnimation.complete()
                terrainAnimation.complete()
            }

            GeoViewCamera {
                id: geoCamera
                objectName: "geoViewCamera"
                viewportSize: Qt.size(sceneRoot.width, sceneRoot.height)
                sceneOrigin: geoScene.sceneOrigin
                // Startup pose: overhead view of a land area with terrain relief so
                // imagery and elevation are visually verifiable (home-position
                // integration comes later). Until this center applies, the camera is
                // unpositioned and SurfaceModel builds nothing (see
                // GeoViewCamera::isPositioned), so the null-island default pose never
                // triggers doomed terrain fetches at (0,0).
                center: QtPositioning.coordinate(47.6329078, -122.0876875)
            }

            GeoScene {
                id: geoScene
                objectName: "geoViewScene"
                camera: geoCamera
            }

            SurfacePatchModel {
                id: patchModel
                objectName: "geoViewPatchModel"
                scene: geoScene
                terrain: true
                // Drape the map imagery the rest of QGC uses (empty disables imagery)
                mapType: QGroundControl.settingsManager.flightMapSettings.mapProvider.rawValue
                         + " " + QGroundControl.settingsManager.flightMapSettings.mapType.rawValue
            }

            View3D {
                id: viewport
                objectName: "geoViewViewport"
                anchors.fill: parent

                environment: SceneEnvironment {
                    clearColor: "#1a2028"
                    backgroundMode: SceneEnvironment.Color
                    // Distance haze toward the background color hides the far-field
                    // LOD rings (terrain blend band and patch range edge)
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

                // Scene camera fully driven by the GeoViewCamera pose. Clip planes
                // scale with distance so the surface stays visible from close-up
                // through whole-earth views.
                PerspectiveCamera {
                    id: sceneCamera
                    objectName: "geoViewSceneCamera"
                    position: geoCamera.scenePosition
                    rotation: geoCamera.sceneRotation
                    fieldOfView: geoCamera.fieldOfView
                    clipNear: Math.max(1, geoCamera.distance / 1000)
                    // Ray length to the farthest retained ground point is at most
                    // maxRange + camera height <= (maxRangeMultiplier + 1) * distance
                    clipFar: Math.max(100000, geoCamera.distance * (patchModel.maxRangeMultiplier + 1))
                }

                // One surface patch per active SurfaceModel entry: draped tile
                // imagery once delivered, LOD-colored fallback while loading
                Repeater3D {
                    model: patchModel
                    delegate: Model {
                        id: patchDelegate
                        objectName: "geoViewPatchDelegate"
                        required property real centerX
                        required property real centerY
                        required property real span
                        required property int zoomLevel
                        required property var heights
                        required property bool covered
                        required property var tileImage
                        required property bool hasTileImage

                        // A covered pending patch stays hidden: its empty flat grid
                        // would z-fight the retained retiring cover
                        visible: !covered
                        position: Qt.vector3d(centerX, centerY, 0)
                        // z-scale converts true-meter heights into mercator scene units
                        // (verticalScale) and animates terrain flat<->full during the
                        // 2D/3D mode transition (terrainScale) without geometry rebuilds.
                        // Never exactly 0: a singular scale breaks the normal matrix.
                        scale: Qt.vector3d(1, 1, Math.max(0.0001, sceneRoot.terrainScale * geoScene.verticalScale))
                        geometry: PatchGeometry {
                            gridSize: patchModel.gridSize
                            span: patchDelegate.span
                            heights: patchDelegate.heights
                        }

                        Texture {
                            id: patchTexture
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
                                diffuseColor: {
                                    if (patchDelegate.hasTileImage) {
                                        return "white"
                                    }
                                    if (patchModel.mapType !== "") {
                                        return "#3a4048"
                                    }
                                    const parity = (Math.round(patchDelegate.centerX / patchDelegate.span)
                                                    + Math.round(patchDelegate.centerY / patchDelegate.span)) & 1
                                    return Qt.hsla((patchDelegate.zoomLevel * 0.13) % 1.0, 0.5,
                                                   parity ? 0.45 : 0.6, 1.0)
                                }
                            }
                        ]
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
                            sceneRoot.completeCameraAnimations()
                            geoCamera.beginPan(centroid.pressPosition)
                        }
                    }
                    onCentroidChanged: {
                        if (active) {
                            geoCamera.panTo(centroid.position)
                        }
                    }
                }

                // Right-drag orbits: full width = 360 deg heading, full height = 180 deg tilt
                DragHandler {
                    id: orbitHandler
                    target: null
                    acceptedButtons: Qt.RightButton
                    onActiveChanged: {
                        if (active) {
                            sceneRoot.completeCameraAnimations()
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
                        sceneRoot.completeCameraAnimations()
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
                            sceneRoot.completeCameraAnimations()
                        }
                    }
                    onScaleChanged: (delta) => geoCamera.zoomBy(1 / delta, centroid.position)
                    onRotationChanged: (delta) => geoCamera.rotateBy(delta, centroid.position)
                }
            }

            // Debug overlay: live SurfaceModel stats for manual verification
            QGCLabel {
                objectName: "geoViewDebugOverlay"
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.margins: ScreenTools.defaultFontPixelWidth
                font.family: ScreenTools.fixedFontFamily
                text: qsTr("patches: %1  pending: %2  max zoom: %3")
                          .arg(patchModel.patchCount)
                          .arg(patchModel.pendingCount)
                          .arg(patchModel.maxZoomLevel)
            }

            NumberAnimation {
                id: tiltAnimation
                target: geoCamera
                property: "tilt"
                duration: sceneRoot.cameraAnimationMs
                easing.type: Easing.InOutQuad
            }

            NumberAnimation {
                id: headingAnimation
                target: geoCamera
                property: "heading"
                duration: sceneRoot.cameraAnimationMs
                easing.type: Easing.InOutQuad
            }

            NumberAnimation {
                id: terrainAnimation
                target: sceneRoot
                property: "terrainScale"
                duration: sceneRoot.cameraAnimationMs
                easing.type: Easing.InOutQuad
            }

            // Single transition path for every mode writer (button, goTo2D/3D,
            // C++): mode changes always animate tilt and terrain to the mode's
            // targets, so camera state and terrain displacement cannot desync
            Connections {
                target: geoCamera
                function onModeChanged() {
                    const to3D = geoCamera.mode === GeoViewCamera.Mode3D
                    tiltAnimation.stop()
                    tiltAnimation.to = to3D ? geoCamera.default3DTilt : 0
                    tiltAnimation.start()
                    terrainAnimation.stop()
                    terrainAnimation.to = to3D ? 1 : 0
                    terrainAnimation.start()
                }
            }

            Column {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: ScreenTools.defaultFontPixelWidth
                spacing: ScreenTools.defaultFontPixelHeight / 2

                // 2D<->3D mode switch: the mode flips immediately (2D locks tilt
                // gestures); the mode-change handler animates tilt and terrain
                QGCButton {
                    objectName: "geoViewModeButton"
                    text: geoCamera.mode === GeoViewCamera.Mode2D ? qsTr("3D") : qsTr("2D")
                    onClicked: geoCamera.mode = (geoCamera.mode === GeoViewCamera.Mode2D)
                                   ? GeoViewCamera.Mode3D : GeoViewCamera.Mode2D
                }

                // Compass: needle tracks heading, click animates back to north-up
                QGCButton {
                    objectName: "geoViewCompassButton"
                    text: qsTr("N")
                    rotation: -geoCamera.heading
                    onClicked: {
                        headingAnimation.stop()
                        // Wrap through 360 for the short way home; the setter
                        // normalizes the final value back to 0
                        headingAnimation.to = (geoCamera.heading > 180) ? 360 : 0
                        headingAnimation.start()
                    }
                }
            }
        }
    }
}

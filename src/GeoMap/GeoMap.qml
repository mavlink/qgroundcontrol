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
import QGroundControl.Controls
import QGroundControl.GeoMap

/// Experimental 2D/3D map control (preview feature): the GeoMap-engine
/// counterpart of FlightMap. Renders the LOD surface patch quadtree with full
/// Google Earth-style camera gestures (pan, orbit, zoom, twist, first-person
/// look) and no view chrome.
Item {
    id: root

    // Keep map item overlays (GeoMapItem) inside the viewport, matching
    // QtLocation Map behavior
    clip: true

    property alias camera: geoCamera

    // Auto-centering (parity with FlightMap): one-shot GCS/vehicle centering
    // plus vehicle following, all decided by the shared MapPositionTracker
    property alias allowGCSLocationCenter: _positionTracker.allowGCSLocationCenter
    property alias allowVehicleLocationCenter: _positionTracker.allowVehicleLocationCenter
    property alias keepVehicleCentered: _positionTracker.keepVehicleCentered
    property alias positionTracker: _positionTracker

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

    readonly property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    readonly property var _activeVehicleCoordinate: _activeVehicle ? _activeVehicle.coordinate : QtPositioning.coordinate()
    // Vehicle items render at coordinate.altitude + home terrain bias (see
    // GeoMapVehicleItem): inset-follow must track that same rendered point or
    // the recenter target is vertically off by the bias under a tilted camera
    readonly property var _trackedVehicleCoordinate: QtPositioning.coordinate(_activeVehicleCoordinate.latitude,
                                                                              _activeVehicleCoordinate.longitude,
                                                                              _activeVehicleCoordinate.altitude + _homeTerrainBias)

    // DEM height minus vehicle-reported AMSL, sampled at home (same math as
    // GeoMapVehicleItem). terrainHeightAt is not a binding dependency, so
    // re-sampled explicitly below as terrain tiles arrive and when home moves.
    property real _homeTerrainBias: 0

    function _updateHomeTerrainBias() {
        if (_activeVehicle && _activeVehicle.homePosition.isValid && !isNaN(_activeVehicle.homePosition.altitude)) {
            _homeTerrainBias = patchModel.terrainHeightAt(_activeVehicle.homePosition) - _activeVehicle.homePosition.altitude
        } else {
            _homeTerrainBias = 0
        }
    }

    on_ActiveVehicleChanged: _updateHomeTerrainBias()

    // Render-statistics overlay (FPS, frame timing, draw calls, texture/mesh
    // memory) for perf work; owned here because it needs the internal View3D
    property bool renderStats: false

    // Terrain displacement factor (see GeoScene.terrainScale)
    property alias terrainScale: geoScene.terrainScale

    // Scene wiring for map items (GeoMapItem consumers)
    readonly property GeoScene scene: geoScene
    readonly property SurfacePatchModel surfaceModel: patchModel

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
        // A recenter jumped to its end would fight the starting gesture; drop it
        recenterAnimation.stop()
    }

    // Keep the camera's look-at point riding the rendered surface: without
    // this the orbit center stays at z=0 and a close-zoom 2D->3D switch over
    // high terrain puts the camera under the mesh (blank view)
    function _updateCenterElevation() {
        geoCamera.centerElevation = patchModel.terrainHeightAt(geoCamera.center)
                                    * geoScene.verticalScale * geoScene.terrainScale
    }

    // Scene-space z of the rendered surface under a screen point, so gestures
    // anchor to the terrain the user actually clicked instead of the z=0
    // plane far beneath it. Sample at the ground-plane hit, then refine once
    // at that elevation (same two-step solve as onRecenterVehicleTo).
    function _surfaceZAt(screenPos) {
        const coord = geoCamera.coordinateAtScreenPoint(screenPos)
        if (!coord.isValid) {
            return 0
        }
        const zScale = geoScene.verticalScale * geoScene.terrainScale
        const z = patchModel.terrainHeightAt(coord) * zScale
        const refined = geoCamera.coordinateAtScreenPoint(screenPos, z)
        return refined.isValid ? (patchModel.terrainHeightAt(refined) * zScale) : z
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
        content3D: mapContent3D
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

    MapPositionTracker {
        id: _positionTracker

        gcsPosition: QGroundControl.qgcPositionManger.gcsPosition
        vehicleCoordinate: root._activeVehicleCoordinate
        centerGCSWhenVehicleValid: QGroundControl.settingsManager.flyViewSettings.keepMapCenteredOnVehicle.rawValue
        userInteracting: panHandler.active || orbitHandler.active || shiftOrbitHandler.active
                         || lookHandler.active || metaLookHandler.active || pinchHandler.active
        animating: recenterAnimation.running

        onCenterMap: (coordinate, firstPosition) => {
            recenterAnimation.stop()
            geoCamera.center = coordinate
            if (firstPosition) {
                geoCamera.distance = geoCamera.distanceForZoomLevel(QGroundControl.flightMapInitialZoom)
            }
        }

        onRecenterVehicleTo: (screenPoint) => {
            recenterAnimation.stop()
            recenterAnimation.from = geoCamera.center
            // The solve assumes the camera pivot elevation stays put, but the
            // pivot rides the terrain (_updateCenterElevation): re-solve at
            // the elevation the pivot will settle to at the destination
            let target = geoScene.centerForCoordinateAtScreenPoint(root._trackedVehicleCoordinate, screenPoint)
            target = geoScene.centerForCoordinateAtScreenPoint(root._trackedVehicleCoordinate, screenPoint,
                                                               patchModel.terrainHeightAt(target))
            recenterAnimation.to = target
            recenterAnimation.start()
        }
    }

    // Inset-follow evaluation: no view chrome here yet, so the unobstructed
    // center rect is the full viewport and there are no corner rects
    Timer {
        interval: 500
        running: root.visible
        repeat: true
        onTriggered: {
            const screenPos = geoScene.screenPositionFor(root._trackedVehicleCoordinate)
            // Unprojectable (behind camera) counts as off-screen: recenter
            const vehiclePoint = (screenPos === undefined) ? Qt.point(-1, -1) : screenPos
            _positionTracker.evaluateInsetFollow(vehiclePoint, Qt.rect(0, 0, root.width, root.height), [])
        }
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
            fieldOfView: geoCamera.verticalFieldOfView
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
            scale: Qt.vector3d(1, 1, Math.max(0.0001, geoScene.terrainScale * geoScene.verticalScale))

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

        // Map item 3D content (GeoMapItem delegate3D instances). Sibling of
        // the terrain node: items pre-scale their own z, and the terrain
        // z-scale would squash models during the 2D/3D transition.
        Node {
            id: mapContent3D
        }

        // Gestures (Google Earth semantics): the ground point under the cursor
        // at gesture start stays under the cursor throughout. Any gesture
        // completes a running mode/compass animation to its end state so
        // the two never fight over the same pose properties.
        DragHandler {
            id: panHandler
            target: null
            acceptedButtons: Qt.LeftButton
            // Plain drag only: Shift/Ctrl+left-drag are the orbit/look gestures below
            acceptedModifiers: Qt.NoModifier
            onActiveChanged: {
                if (active) {
                    root.completeCameraAnimations()
                    // Anchor at the current position, not pressPosition: after a
                    // two-finger pinch drops to one finger this handler re-activates
                    // with a stale pressPosition, and anchoring there jumps the map.
                    geoCamera.beginPan(centroid.position, root._surfaceZAt(centroid.position))
                }
            }
            onCentroidChanged: {
                if (active) {
                    geoCamera.panTo(centroid.position)
                }
            }
        }

        // Right/middle-drag orbits about the pressed ground point: full width =
        // 360 deg heading, full height = 180 deg tilt.
        // Mouse/touchpad only: acceptedButtons doesn't filter touch points, so
        // without acceptedDevices this handler steals single-finger drags from
        // panHandler on touchscreens (touch orbits via PinchHandler twist instead).
        // TouchPad keeps trackpad secondary-click drag working.
        DragHandler {
            id: orbitHandler
            target: null
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            acceptedButtons: Qt.RightButton | Qt.MiddleButton
            // Unmodified only: macOS synthesizes Ctrl+left-click as a right-click
            // (carrying MetaModifier), which must fall through to metaLookHandler
            acceptedModifiers: Qt.NoModifier
            onActiveChanged: {
                if (active) {
                    root.completeCameraAnimations()
                    geoCamera.beginOrbit(centroid.pressPosition, root._surfaceZAt(centroid.pressPosition))
                    // Apply motion accumulated before activation: a short drag
                    // can activate and release with no further centroid change
                    geoCamera.orbitTo(centroid.position)
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
            // Default acceptedDevices=Mouse drops trackpad scroll (and mouse
            // wheel on Wayland/xcb, which misreport as TouchPad) — see the
            // FlightMap WheelHandler comment for the full platform rundown
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            onWheel: (event) => {
                root.completeCameraAnimations()
                geoCamera.zoom(event.angleDelta.y, point.position)
            }
        }

        // Shift+left-drag orbits about the pressed ground point, same as
        // right/middle-drag (keyboard-modifier alternative for one-button mice)
        DragHandler {
            id: shiftOrbitHandler
            target: null
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            acceptedButtons: Qt.LeftButton
            acceptedModifiers: Qt.ShiftModifier
            onActiveChanged: {
                if (active) {
                    root.completeCameraAnimations()
                    geoCamera.beginOrbit(centroid.pressPosition, root._surfaceZAt(centroid.pressPosition))
                    // Apply motion accumulated before activation (see orbitHandler)
                    geoCamera.orbitTo(centroid.position)
                }
            }
            onCentroidChanged: {
                if (active) {
                    geoCamera.orbitTo(centroid.position)
                }
            }
        }

        // Ctrl+left-drag is first-person look: the camera stays put and the
        // view rotates, like turning your head (Google Earth Ctrl+drag).
        // Drag toward where you want to look.
        DragHandler {
            id: lookHandler
            target: null
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            acceptedButtons: Qt.LeftButton
            acceptedModifiers: Qt.ControlModifier
            onActiveChanged: {
                if (active) {
                    root.completeCameraAnimations()
                    geoCamera.beginLook(centroid.pressPosition)
                    // Apply motion accumulated before activation (see orbitHandler)
                    geoCamera.lookTo(centroid.position)
                }
            }
            onCentroidChanged: {
                if (active) {
                    geoCamera.lookTo(centroid.position)
                }
            }
        }

        // macOS delivers the physical Ctrl key as Qt.MetaModifier (Qt swaps
        // Ctrl/Cmd), so accept it too: Ctrl+drag looks on every platform
        // (and Cmd+drag still works via lookHandler, matching Google Earth).
        // RightButton included because macOS synthesizes Ctrl+left-click as a
        // right-click, so that's the button this drag actually arrives on.
        DragHandler {
            id: metaLookHandler
            target: null
            enabled: Qt.platform.os === "osx"
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            acceptedModifiers: Qt.MetaModifier
            onActiveChanged: {
                if (active) {
                    root.completeCameraAnimations()
                    geoCamera.beginLook(centroid.pressPosition)
                    // Apply motion accumulated before activation (see orbitHandler)
                    geoCamera.lookTo(centroid.position)
                }
            }
            onCentroidChanged: {
                if (active) {
                    geoCamera.lookTo(centroid.position)
                }
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

    // Pivot ring (Google Earth-style): marks the ground point an orbit drag
    // rotates about, visible only while the drag is active. The pivot stays
    // pinned to its press position on screen, so the ring never moves.
    Rectangle {
        id: orbitPivotIndicator
        objectName: "geoMapOrbitPivotIndicator"

        readonly property point _pivot: orbitHandler.active ? orbitHandler.centroid.pressPosition
                                                            : shiftOrbitHandler.centroid.pressPosition

        visible: orbitHandler.active || shiftOrbitHandler.active
        x: _pivot.x - (width / 2)
        y: _pivot.y - (height / 2)
        width: ScreenTools.defaultFontPixelHeight * 1.5
        height: width
        radius: width / 2
        color: "transparent"
        // Dark halo keeps the white ring visible over light imagery
        border.color: Qt.rgba(0, 0, 0, 0.4)
        border.width: (ScreenTools.defaultFontPixelHeight / 8) + 2

        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: width / 2
            color: "transparent"
            border.color: "white"
            border.width: ScreenTools.defaultFontPixelHeight / 8
        }

        Rectangle {
            anchors.centerIn: parent
            width: ScreenTools.defaultFontPixelHeight / 4
            height: width
            radius: width / 2
            color: "white"
        }
    }

    /// Ground station location (parity with the FlightMap marker)
    GeoMapItem {
        id: gcsIndicator

        scene: geoScene
        surfaceModel: patchModel
        coordinate: gcsIndicator._gcsPosition
        anchorPoint: Qt.point(gcsImage.width / 2, gcsImage.height / 2)
        width: gcsImage.width
        height: gcsImage.height
        visible: gcsIndicator._gcsPosition.isValid

        readonly property var _gcsPosition: QGroundControl.qgcPositionManger.gcsPosition
        readonly property real _gcsHeading: QGroundControl.qgcPositionManger.gcsHeading

        Image {
            id: gcsImage
            source: isNaN(gcsIndicator._gcsHeading) ? "/res/QGCLogoFull.svg" : "/res/QGCLogoArrow.svg"
            mipmap: true
            antialiasing: true
            fillMode: Image.PreserveAspectFit
            height: ScreenTools.defaultFontPixelHeight * (isNaN(gcsIndicator._gcsHeading) ? 1.75 : 2.5)
            sourceSize.height: height
            transform: Rotation {
                origin.x: gcsImage.width / 2
                origin.y: gcsImage.height / 2
                // Camera heading rotates map north on screen; the arrow follows
                angle: isNaN(gcsIndicator._gcsHeading) ? 0 : gcsIndicator._gcsHeading + geoCamera.heading
            }
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
        target: geoScene
        property: "terrainScale"
        duration: root.cameraAnimationMs
        easing.type: Easing.InOutQuad
    }

    CoordinateAnimation {
        id: recenterAnimation
        target: geoCamera
        property: "center"
        duration: 1000
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
        function onCenterChanged() {
            root._updateCenterElevation()
        }
    }

    Connections {
        target: patchModel
        function onTerrainHeightsChanged() {
            root._updateCenterElevation()
            root._updateHomeTerrainBias()
        }
    }

    Connections {
        target: geoScene
        function onTerrainScaleChanged() {
            root._updateCenterElevation()
        }
        // verticalScale depends on the scene origin
        function onSceneOriginChanged() {
            root._updateCenterElevation()
        }
    }

    Connections {
        target: root._activeVehicle
        function onHomePositionChanged() {
            root._updateHomeTerrainBias()
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes
import QtQuick3D
import QtQuick.Dialogs
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView
import QGroundControl.FlightMap
import QGroundControl.GeoMap
import QGroundControl.Toolbar
import QGroundControl.Viewer3D

import ATLAS.Kml 1.0

Item {
    id: _root

    readonly property bool _is3DMode: QGCViewer3DManager.displayMode === QGCViewer3DManager.View3D
    readonly property bool _keepSceneAlive: QGroundControl.settingsManager.viewer3DSettings.keepSceneAlive.rawValue

    property var planController: _planController
    property var guidedController: _guidedController

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property var _missionController: _planController.missionController
    property var _guidedController: guidedActionsController
    property var _guidedValueSlider: guidedValueSlider
    property var _mapControl: mapEngineLoader.item
    property var _atlasKmlOverlayItem: null

    property bool _mainWindowIsMap: _mapControl ? _mapControl.pipState.state === _mapControl.pipState.fullState : true
    property bool _isFullWindowItemDark: _mainWindowIsMap ? (_mapControl ? _mapControl.isSatelliteMap : true) : true

    property real _toolsMargin: 8
    property real _mapPipWidth: Math.min(292, Math.max(252, width * 0.175))
    property real _mapPipHeight: Math.min(154, Math.max(148, (height - toolbar.height) * 0.18))

    // ATLAS 2D/3D preferences are independent for the miniature map and the
    // expanded map. They persist between launches. The 3D mode uses QGC's
    // native GeoMap terrain engine (satellite imagery draped over elevation).
    property bool _miniMap3D: QGroundControl.loadBoolGlobalSetting("AtlasMiniMap3D", false)
    property bool _fullMap3D: QGroundControl.loadBoolGlobalSetting("AtlasFullMap3D", false)
    readonly property bool _mapWants3D: _mainWindowIsMap ? _fullMap3D : _miniMap3D

    function dropMainStatusIndicatorTool() {
        toolbar.dropMainStatusIndicatorTool()
    }

    function centerMapOnVehicle() {
        if (_mapControl && _activeVehicle) {
            _mapControl.center = _activeVehicle.coordinate
        }
    }

    // -----------------------------------------------------------------
    // ATLAS map interaction helpers
    // -----------------------------------------------------------------

    function prepareInteractiveMiniMap() {
        if (!_mapControl) {
            return
        }

        // FlyViewMap normally forces the vehicle to remain centered while
        // it is in PiP. For ATLAS the mini-map is intentionally interactive.
        try {
            if (_mapControl._keepVehicleCentered !== undefined) {
                _mapControl._keepVehicleCentered = false
            }
        } catch (e) {
        }

        // GeoMap uses a different internal camera object. Disable its PiP
        // follow behaviour as soon as the operator starts interacting.
        try {
            if (_mapControl.geoMap && _mapControl.geoMap.keepVehicleCentered !== undefined) {
                _mapControl.geoMap.keepVehicleCentered = false
            }
        } catch (e) {
        }
    }

    function changeMapZoom(delta) {
        if (!_mapControl) {
            return
        }

        prepareInteractiveMiniMap()

        // GeoMap adapter: zoomLevel is read-only, so change camera distance.
        try {
            if (_mapControl.geoMap && _mapControl.geoMap.camera) {
                var camera = _mapControl.geoMap.camera
                var level = camera.zoomLevelForDistance(camera.distance)
                level = Math.max(2, Math.min(23, level + delta))
                camera.distance = camera.distanceForZoomLevel(level)
                return
            }
        } catch (e) {
        }

        // QtLocation map.
        _mapControl.zoomLevel = Math.max(2, Math.min(23, _mapControl.zoomLevel + delta))
    }

    function panMiniMap(dx, dy) {
        if (!_mapControl || Math.abs(dx) + Math.abs(dy) < 0.1) {
            return
        }

        prepareInteractiveMiniMap()

        var newCenter = _mapControl.toCoordinate(
            Qt.point(_mapControl.width / 2 - dx, _mapControl.height / 2 - dy),
            false
        )

        if (newCenter && newCenter.isValid) {
            _mapControl.center = newCenter
        }
    }

    function rotateMiniMap(dx, dy) {
        if (!_mapControl || !_mapControl.geoMap || !_mapControl.geoMap.camera) {
            return
        }

        var camera = _mapControl.geoMap.camera
        camera.heading = camera.heading + dx * 0.45
        camera.tilt = Math.max(25, Math.min(80, camera.tilt - dy * 0.35))
    }

    function applyAtlasMapDimension() {
        if (!_mapControl || !_mapControl.geoMap || !_mapControl.geoMap.camera) {
            return
        }

        var camera = _mapControl.geoMap.camera
        camera.mode = _mapWants3D ? GeoMapCamera.Mode3D : GeoMapCamera.Mode2D
    }

    function setAtlasMap3D(enabled) {
        if (_mainWindowIsMap) {
            _fullMap3D = enabled
            QGroundControl.saveBoolGlobalSetting("AtlasFullMap3D", enabled)
        } else {
            _miniMap3D = enabled
            QGroundControl.saveBoolGlobalSetting("AtlasMiniMap3D", enabled)
        }

        applyAtlasMapDimension()
    }

    function resetMapNorth() {
        if (_mapControl && _mapControl.geoMap) {
            _mapControl.geoMap.animateHeadingToNorth()
        }
    }

    function focusKmlZones() {
        if (!_mapControl || !atlasKml.loaded || !atlasKml.centerCoordinate.isValid) {
            return
        }
        _mapControl.center = atlasKml.centerCoordinate
        try {
            if (_mapControl.geoMap && _mapControl.geoMap.camera) {
                var span = Math.max(atlasKml.latitudeSpan, atlasKml.longitudeSpan)
                if (span > 0.000001) {
                    var zoom = Math.log(360.0 / span) / Math.LN2 - 1.2
                    zoom = Math.max(4, Math.min(18, zoom))
                    var camera = _mapControl.geoMap.camera
                    camera.distance = camera.distanceForZoomLevel(zoom)
                }
            }
        } catch (e) {
        }
    }

    function swapMapVideo() {
        if (_pipView && _mapControl && videoControl) {
            _pipView._swapPip()
        }
    }

    on_MainWindowIsMapChanged: atlasMapModeApplyTimer.restart()

    PlanMasterController {
        id: _planController
        flyView: true
        Component.onCompleted: start()
    }

    QGCToolInsets {
        id: payloadBaseInsets
        topEdgeLeftInset: toolbar.height
        topEdgeCenterInset: toolbar.height
        topEdgeRightInset: toolbar.height
        leftEdgeBottomInset: _pipView.visible ? _pipView.x + _pipView.width + _toolsMargin : 0
        bottomEdgeLeftInset: _pipView.visible ? _pipView.height + _toolsMargin : 0
    }

    Item {
        id: mapHolder
        anchors.fill: parent

        Loader {
            id: mapEngineLoader

            // ATLAS requires the terrain-capable GeoMap engine so the operator
            // can switch 2D <-> 3D both in the miniature and expanded map.
            Component.onCompleted: sourceComponent = geoMapEngineComponent
            onLoaded: {
                if (_root._atlasKmlOverlayItem) {
                    _root._atlasKmlOverlayItem.destroy()
                    _root._atlasKmlOverlayItem = null
                }
                if (item && item.geoMap) {
                    _root._atlasKmlOverlayItem = atlasKmlOverlayComponent.createObject(item)
                }
                Qt.callLater(_root.applyAtlasMapDimension)
            }
        }

        Component {
            id: qtLocationEngineComponent

            FlyViewMap {
                objectName: "flyViewMap"
                planMasterController: _planController
                rightPanelWidth: ScreenTools.defaultFontPixelHeight * 9
                pipView: _pipView
                pipMode: !_mainWindowIsMap
                toolInsets: customOverlay.totalToolInsets
                mapName: "FlightDisplayView"
                enabled: !_is3DMode
                visible: !_is3DMode
            }
        }

        Component {
            id: geoMapEngineComponent

            FlyViewGeoMapAdapter {
                objectName: "flyViewGeoMapAdapter"
                planMasterController: _planController
                rightPanelWidth: ScreenTools.defaultFontPixelHeight * 9
                pipView: _pipView
                pipMode: !_mainWindowIsMap
                toolInsets: customOverlay.totalToolInsets
                toolInsetsTopOffset: toolbar.height + _toolsMargin
                mapName: "FlightDisplayView"
                enabled: !_is3DMode
                visible: !_is3DMode
            }
        }

        // -----------------------------------------------------------------
        // KML / AIRSPACE OVERLAY
        //
        // Unlike v0.12.0, this renderer does not invent one red style or a
        // fallback height. Every geometry uses the KML's own style, altitude
        // mode, extrusion flag and coordinate altitude. It also renders
        // LineString and Point placemarks, not only polygons.
        // -----------------------------------------------------------------
        Component {
            id: atlasKmlOverlayComponent

            Item {
                id: atlasKmlOverlay
                anchors.fill: parent
                visible: atlasKml.visible && atlasKml.loaded
                enabled: false
                z: 20

                readonly property var geoMap: parent && parent.geoMap ? parent.geoMap : null
                property int terrainRevision: 0
                readonly property bool terrain3DActive: geoMap && geoMap.scene && geoMap.scene.terrainScale > 0.02

                function terrainHeight(coordinate) {
                    terrainRevision
                    if (!geoMap || !geoMap.surfaceModel || !coordinate || !coordinate.isValid) {
                        return 0
                    }
                    var value = geoMap.surfaceModel.terrainDataHeightAt(coordinate)
                    return isNaN(value) ? 0 : value
                }

                function groundCoordinate(coordinate) {
                    if (!coordinate || !coordinate.isValid) {
                        return QtPositioning.coordinate()
                    }
                    return QtPositioning.coordinate(
                        coordinate.latitude,
                        coordinate.longitude,
                        terrainHeight(coordinate))
                }

                function topCoordinate(coordinate, altitudeModeCode) {
                    if (!coordinate || !coordinate.isValid) {
                        return QtPositioning.coordinate()
                    }

                    if (altitudeModeCode === 2) {
                        // KML absolute: coordinate altitude is AMSL.
                        return QtPositioning.coordinate(
                            coordinate.latitude,
                            coordinate.longitude,
                            isNaN(coordinate.altitude) ? terrainHeight(coordinate) : coordinate.altitude)
                    }

                    if (altitudeModeCode === 1) {
                        // KML relativeToGround: coordinate altitude is AGL.
                        var agl = isNaN(coordinate.altitude) ? 0 : coordinate.altitude
                        return QtPositioning.coordinate(
                            coordinate.latitude,
                            coordinate.longitude,
                            terrainHeight(coordinate) + agl)
                    }

                    return groundCoordinate(coordinate)
                }

                function topCoordinates(feature) {
                    terrainRevision
                    if (!feature || !feature.coordinates) {
                        return []
                    }

                    if (feature.altitudeModeCode === 0) {
                        // GeoMapProjectedPath will clamp these to terrain.
                        return feature.coordinates
                    }

                    var result = []
                    for (var i = 0; i < feature.coordinates.length; ++i) {
                        result.push(topCoordinate(feature.coordinates[i], feature.altitudeModeCode))
                    }
                    return result
                }

                Connections {
                    target: atlasKmlOverlay.geoMap ? atlasKmlOverlay.geoMap.surfaceModel : null

                    function onTerrainHeightsChanged() {
                        // relativeToGround KML heights must follow refined DEM
                        // tiles, just like Google Earth terrain-relative items.
                        atlasKmlOverlay.terrainRevision++
                    }
                }

                // ---------------------------------------------------------
                // POLYGONS
                // ---------------------------------------------------------
                Repeater {
                    model: atlasKml.polygons

                    Item {
                        id: polygonVisual
                        anchors.fill: parent
                        visible: modelData.visible
                        enabled: false

                        readonly property var feature: modelData
                        readonly property var topCoordinates: {
                            atlasKmlOverlay.terrainRevision
                            return atlasKmlOverlay.topCoordinates(feature)
                        }
                        readonly property int projectedAltitudeMode:
                            feature.altitudeModeCode === 0
                            ? GeoMapItem.ClampToGround
                            : GeoMapItem.Absolute

                        // Top/footprint. In 2D all altitude collapses naturally
                        // with the GeoMap terrainScale. In 3D it sits at the
                        // exact KML absolute/relative altitude.
                        GeoMapPolygon {
                            scene: atlasKmlOverlay.geoMap ? atlasKmlOverlay.geoMap.scene : null
                            surfaceModel: atlasKmlOverlay.geoMap ? atlasKmlOverlay.geoMap.surfaceModel : null
                            coordinates: polygonVisual.topCoordinates
                            altitudeMode: polygonVisual.projectedAltitudeMode
                            strokeColor: polygonVisual.feature.outlineEnabled
                                         ? polygonVisual.feature.lineColor
                                         : "transparent"
                            strokeWidth: polygonVisual.feature.lineWidth
                            fillColor: polygonVisual.feature.fillEnabled
                                       ? polygonVisual.feature.fillColor
                                       : "transparent"
                            extrudeHeightMeters: 0
                        }

                        // Google Earth shows the lower footprint for extruded
                        // outline-only shapes. Keep it out of flat 2D mode.
                        GeoMapPolygon {
                            visible: atlasKmlOverlay.terrain3DActive &&
                                     polygonVisual.feature.extrude &&
                                     polygonVisual.feature.outlineEnabled &&
                                     polygonVisual.feature.altitudeModeCode !== 0
                            scene: atlasKmlOverlay.geoMap ? atlasKmlOverlay.geoMap.scene : null
                            surfaceModel: atlasKmlOverlay.geoMap ? atlasKmlOverlay.geoMap.surfaceModel : null
                            coordinates: polygonVisual.feature.coordinates
                            altitudeMode: GeoMapItem.ClampToGround
                            strokeColor: polygonVisual.feature.lineColor
                            strokeWidth: polygonVisual.feature.lineWidth
                            fillColor: "transparent"
                            extrudeHeightMeters: 0
                        }

                        // Filled KML polygons get real terrain-to-top 3D walls.
                        // Absolute means AMSL, relative means AGL. No fallback
                        // height is substituted.
                        GeoMapItem {
                            id: filledWallItem
                            visible: polygonVisual.feature.extrude &&
                                     polygonVisual.feature.fillEnabled &&
                                     polygonVisual.feature.altitudeModeCode !== 0
                            scene: atlasKmlOverlay.geoMap ? atlasKmlOverlay.geoMap.scene : null
                            surfaceModel: atlasKmlOverlay.geoMap ? atlasKmlOverlay.geoMap.surfaceModel : null
                            altitudeMode: GeoMapItem.Absolute
                            crossfade3D: false

                            property var _wallGeometry: null

                            coordinate: (_wallGeometry && _wallGeometry.anchorCoordinate.isValid)
                                        ? _wallGeometry.anchorCoordinate
                                        : QtPositioning.coordinate()

                            delegate3D: Component {
                                Node {
                                    scale: Qt.vector3d(
                                        1,
                                        1,
                                        atlasKmlOverlay.geoMap && atlasKmlOverlay.geoMap.scene
                                        ? atlasKmlOverlay.geoMap.scene.terrainScale
                                        : 1)

                                    Model {
                                        geometry: AtlasKmlWallGeometry {
                                            id: polygonWallGeometry
                                            scene: atlasKmlOverlay.geoMap ? atlasKmlOverlay.geoMap.scene : null
                                            surfaceModel: atlasKmlOverlay.geoMap ? atlasKmlOverlay.geoMap.surfaceModel : null
                                            coordinates: filledWallItem.visible ? polygonVisual.feature.coordinates : []
                                            altitudeMode: polygonVisual.feature.altitudeModeCode
                                            closed: true
                                        }

                                        materials: PrincipledMaterial {
                                            lighting: PrincipledMaterial.NoLighting
                                            baseColor: polygonVisual.feature.fillColor
                                            alphaMode: PrincipledMaterial.Blend
                                            cullMode: Material.NoCulling
                                        }

                                        Component.onCompleted: filledWallItem._wallGeometry = polygonWallGeometry
                                        Component.onDestruction: filledWallItem._wallGeometry = null
                                    }
                                }
                            }
                        }

                        // fill=0 in KML means outline/wireframe. This is the
                        // important ARTEX case: vertical edges are lines, not
                        // giant translucent red curtains.
                        Repeater {
                            model: (polygonVisual.feature.extrude &&
                                    !polygonVisual.feature.fillEnabled &&
                                    polygonVisual.feature.altitudeModeCode !== 0 &&
                                    atlasKmlOverlay.terrain3DActive)
                                   ? polygonVisual.feature.coordinates
                                   : []

                            Item {
                                anchors.fill: parent

                                readonly property var bottomCoordinate:
                                    atlasKmlOverlay.groundCoordinate(modelData)
                                readonly property var topCoordinate:
                                    atlasKmlOverlay.topCoordinate(
                                        modelData,
                                        polygonVisual.feature.altitudeModeCode)

                                GeoMapProjectedPath {
                                    id: polygonVerticalPath
                                    scene: atlasKmlOverlay.geoMap ? atlasKmlOverlay.geoMap.scene : null
                                    surfaceModel: atlasKmlOverlay.geoMap ? atlasKmlOverlay.geoMap.surfaceModel : null
                                    altitudeMode: GeoMapItem.Absolute
                                    coordinates: [bottomCoordinate, topCoordinate]
                                }

                                Shape {
                                    visible: polygonVerticalPath.projected

                                    ShapePath {
                                        strokeColor: polygonVisual.feature.lineColor
                                        strokeWidth: polygonVisual.feature.lineWidth
                                        fillColor: "transparent"

                                        PathPolyline {
                                            path: polygonVerticalPath.screenPoints
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // ---------------------------------------------------------
                // LINE STRINGS
                // ---------------------------------------------------------
                Repeater {
                    model: atlasKml.lines

                    Item {
                        id: lineVisual
                        anchors.fill: parent
                        visible: modelData.visible
                        enabled: false

                        readonly property var feature: modelData
                        readonly property var topCoordinates: {
                            atlasKmlOverlay.terrainRevision
                            return atlasKmlOverlay.topCoordinates(feature)
                        }
                        readonly property int projectedAltitudeMode:
                            feature.altitudeModeCode === 0
                            ? GeoMapItem.ClampToGround
                            : GeoMapItem.Absolute

                        GeoMapProjectedPath {
                            id: lineTopPath
                            scene: atlasKmlOverlay.geoMap ? atlasKmlOverlay.geoMap.scene : null
                            surfaceModel: atlasKmlOverlay.geoMap ? atlasKmlOverlay.geoMap.surfaceModel : null
                            altitudeMode: lineVisual.projectedAltitudeMode
                            coordinates: lineVisual.topCoordinates
                        }

                        Shape {
                            visible: lineTopPath.projected

                            ShapePath {
                                strokeColor: lineVisual.feature.lineColor
                                strokeWidth: lineVisual.feature.lineWidth
                                fillColor: "transparent"

                                PathPolyline {
                                    path: lineTopPath.screenPoints
                                }
                            }
                        }

                        // KML LineString extrusion is shown as wireframe drops
                        // to terrain. This avoids turning operational corridors
                        // into opaque curtains.
                        Repeater {
                            model: (lineVisual.feature.extrude &&
                                    lineVisual.feature.altitudeModeCode !== 0 &&
                                    atlasKmlOverlay.terrain3DActive)
                                   ? lineVisual.feature.coordinates
                                   : []

                            Item {
                                anchors.fill: parent

                                readonly property var bottomCoordinate:
                                    atlasKmlOverlay.groundCoordinate(modelData)
                                readonly property var topCoordinate:
                                    atlasKmlOverlay.topCoordinate(
                                        modelData,
                                        lineVisual.feature.altitudeModeCode)

                                GeoMapProjectedPath {
                                    id: lineVerticalPath
                                    scene: atlasKmlOverlay.geoMap ? atlasKmlOverlay.geoMap.scene : null
                                    surfaceModel: atlasKmlOverlay.geoMap ? atlasKmlOverlay.geoMap.surfaceModel : null
                                    altitudeMode: GeoMapItem.Absolute
                                    coordinates: [bottomCoordinate, topCoordinate]
                                }

                                Shape {
                                    visible: lineVerticalPath.projected

                                    ShapePath {
                                        strokeColor: lineVisual.feature.lineColor
                                        strokeWidth: lineVisual.feature.lineWidth
                                        fillColor: "transparent"

                                        PathPolyline {
                                            path: lineVerticalPath.screenPoints
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // ---------------------------------------------------------
                // POINTS / LABELS
                // ---------------------------------------------------------
                Repeater {
                    model: atlasKml.points

                    Item {
                        id: pointVisual
                        anchors.fill: parent
                        visible: modelData.visible
                        enabled: false

                        readonly property var feature: modelData
                        readonly property var displayCoordinate: {
                            atlasKmlOverlay.terrainRevision
                            return atlasKmlOverlay.topCoordinate(
                                feature.coordinate,
                                feature.altitudeModeCode)
                        }

                        GeoMapItem {
                            scene: atlasKmlOverlay.geoMap ? atlasKmlOverlay.geoMap.scene : null
                            surfaceModel: atlasKmlOverlay.geoMap ? atlasKmlOverlay.geoMap.surfaceModel : null
                            coordinate: pointVisual.displayCoordinate
                            altitudeMode: pointVisual.feature.altitudeModeCode === 0
                                          ? GeoMapItem.ClampToGround
                                          : GeoMapItem.Absolute

                            width: Math.max(18, 26 * pointVisual.feature.iconScale)
                            height: Math.max(18, 26 * pointVisual.feature.iconScale)
                            anchorPoint: Qt.point(width / 2, height / 2)

                            Row {
                                anchors.left: parent.left
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 4

                                Item {
                                    width: Math.max(18, 26 * pointVisual.feature.iconScale)
                                    height: width

                                    Image {
                                        anchors.fill: parent
                                        source: pointVisual.feature.iconHref
                                        visible: pointVisual.feature.iconHref.length > 0
                                        fillMode: Image.PreserveAspectFit
                                        smooth: true
                                        mipmap: true
                                    }

                                    Rectangle {
                                        anchors.centerIn: parent
                                        width: 8
                                        height: 8
                                        radius: 4
                                        visible: pointVisual.feature.iconHref.length === 0
                                        color: pointVisual.feature.lineColor
                                        border.width: 1
                                        border.color: "white"
                                    }
                                }

                                QGCLabel {
                                    visible: pointVisual.feature.name.length > 0 &&
                                             pointVisual.feature.labelScale > 0
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: pointVisual.feature.name
                                    color: pointVisual.feature.labelColor
                                    font.pixelSize: Math.max(
                                        8,
                                        11 * pointVisual.feature.labelScale)
                                    style: Text.Outline
                                    styleColor: "#B0000000"
                                }
                            }
                        }

                        // Point extrusion (rare, but valid KML).
                        Item {
                            anchors.fill: parent
                            visible: pointVisual.feature.extrude &&
                                     pointVisual.feature.altitudeModeCode !== 0 &&
                                     atlasKmlOverlay.terrain3DActive

                            readonly property var bottomCoordinate:
                                atlasKmlOverlay.groundCoordinate(pointVisual.feature.coordinate)

                            GeoMapProjectedPath {
                                id: pointDropPath
                                scene: atlasKmlOverlay.geoMap ? atlasKmlOverlay.geoMap.scene : null
                                surfaceModel: atlasKmlOverlay.geoMap ? atlasKmlOverlay.geoMap.surfaceModel : null
                                altitudeMode: GeoMapItem.Absolute
                                coordinates: [parent.bottomCoordinate, pointVisual.displayCoordinate]
                            }

                            Shape {
                                visible: pointDropPath.projected

                                ShapePath {
                                    strokeColor: pointVisual.feature.lineColor
                                    strokeWidth: Math.max(1, pointVisual.feature.lineWidth)
                                    fillColor: "transparent"

                                    PathPolyline {
                                        path: pointDropPath.screenPoints
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        FlyViewVideo {
            id: videoControl
            pipView: _pipView
        }

        // Em fullscreen a UI ATLAS fica escondida, mas o scroll continua a
        // controlar o zoom da A8. Cliques/double-click continuam entregues
        // ao FlyViewVideo porque este MouseArea não aceita botões.
        Timer {
            id: fullScreenZoomStopTimer
            interval: 120
            repeat: false
            onTriggered: atlasPayload.zoom(0)
        }

        MouseArea {
            anchors.fill: parent
            z: QGroundControl.zOrderTopMost - 2
            enabled: QGroundControl.videoManager.fullScreen && atlasPayload.a8Connected
            acceptedButtons: Qt.NoButton

            onWheel: function(wheel) {
                atlasPayload.zoom(wheel.angleDelta.y > 0 ? 1 : -1)
                fullScreenZoomStopTimer.restart()
                wheel.accepted = true
            }
        }

        // -----------------------------------------------------------------
        // PAYLOAD MINI-MAP
        // -----------------------------------------------------------------
        PipView {
            id: _pipView
            objectName: "flyViewPipView"
            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 10
            item1IsFullSettingsKey: "MainFlyWindowIsMap"
            item1: _mapControl
            item2: QGroundControl.videoManager.hasVideo ? videoControl : null
            show: QGroundControl.videoManager.hasVideo && !QGroundControl.videoManager.fullScreen &&
                  (videoControl.pipState.state === videoControl.pipState.pipState ||
                   (_mapControl && _mapControl.pipState.state === _mapControl.pipState.pipState))
            z: QGroundControl.zOrderWidgets + 20

            _pipSize: _root._mapPipWidth
            height: _root._mapPipHeight

            // O MouseArea nativo do PipView fica desativado porque o ATLAS
            // trata drag/zoom/click explicitamente: drag move o mini-mapa,
            // scroll faz zoom e um click curto expande o mapa.
            enabled: false

            property real leftEdgeBottomInset: visible ? x + width + anchors.bottomMargin : 0
            property real bottomEdgeLeftInset: visible ? height + anchors.bottomMargin : 0
        }

        // FlyViewGeoMapAdapter temporarily forces 2D when PiP state changes.
        // Re-apply the ATLAS preference one event-loop turn later so miniature
        // 3D is a first-class mode instead of being reset during swaps.
        Timer {
            id: atlasMapModeApplyTimer
            interval: 1
            repeat: false
            onTriggered: _root.applyAtlasMapDimension()
        }

        Rectangle {
            anchors.fill: _pipView
            z: _pipView.z + 1
            visible: _pipView.visible
            color: "transparent"
            border.width: 2
            border.color: "#28485B"
            radius: 7
        }



        Row {
            id: miniMapDimensionToggle
            z: _pipView.z + 4
            visible: _pipView.visible && !_root._mainWindowIsMap
            anchors.left: _pipView.left
            anchors.top: _pipView.top
            anchors.leftMargin: 7
            anchors.topMargin: 7
            spacing: 2

            Repeater {
                model: [
                    { "label": "2D", "enabled3D": false },
                    { "label": "3D", "enabled3D": true }
                ]

                Rectangle {
                    width: 38
                    height: 26
                    radius: 5
                    color: (_root._miniMap3D === modelData.enabled3D) ? "#249AF3" : "#D0101B25"
                    border.width: 1
                    border.color: (_root._miniMap3D === modelData.enabled3D) ? "#62C4FF" : "#45677A"

                    QGCLabel {
                        anchors.centerIn: parent
                        text: modelData.label
                        color: "white"
                        font.bold: true
                        font.pixelSize: 12
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: _root.setAtlasMap3D(modelData.enabled3D)
                    }
                }
            }

            Rectangle {
                width: 46
                height: 26
                radius: 5
                color: atlasKml.loaded && atlasKml.visible ? "#A83232" : "#D0101B25"
                border.width: 1
                border.color: atlasKml.loaded && atlasKml.visible ? "#FF6A6A" : "#45677A"

                QGCLabel {
                    anchors.centerIn: parent
                    text: "KML"
                    color: "white"
                    font.bold: true
                    font.pixelSize: 11
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: kmlPopup.open()
                }
            }
        }

        Column {
            z: _pipView.z + 3
            visible: _pipView.visible && !_root._mainWindowIsMap
            anchors.right: _pipView.right
            anchors.rightMargin: 8
            anchors.verticalCenter: _pipView.verticalCenter
            spacing: 5

            Repeater {
                model: [
                    { "text": "◎", "action": "CENTER" },
                    { "text": "+", "action": "PLUS" },
                    { "text": "−", "action": "MINUS" }
                ]

                Rectangle {
                    width: 34
                    height: 34
                    radius: 5
                    color: "#D00B1B26"
                    border.width: 1
                    border.color: "#45677A"

                    QGCLabel {
                        anchors.centerIn: parent
                        text: modelData.text
                        color: "white"
                        font.bold: true
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (!_mapControl) {
                                return
                            }
                            if (modelData.action === "PLUS") {
                                _root.changeMapZoom(1)
                            } else if (modelData.action === "MINUS") {
                                _root.changeMapZoom(-1)
                            } else {
                                _root.centerMapOnVehicle()
                            }
                        }
                    }
                }
            }
        }

        // Na abertura, vídeo é a vista principal. Ao contrário da versão
        // anterior, isto é one-shot: depois o operador pode expandir o mapa
        // sem o sistema o mandar novamente para PiP.
        Timer {
            id: startupVideoPrimaryTimer
            interval: 250
            repeat: true
            running: true

            onTriggered: {
                if (!_mapControl || !videoControl || !QGroundControl.videoManager.hasVideo) {
                    return
                }

                if (_mapControl.pipState.state === _mapControl.pipState.fullState) {
                    _pipView._swapPip()
                }

                stop()
            }
        }

        // -----------------------------------------------------------------
        // INTERAÇÃO DO MINI-MAPA
        //
        // Drag: move o mapa. Scroll: zoom. Click curto: expande mapa e
        // coloca o vídeo no PiP. Apenas os controlos de navegação ficam por cima.
        // -----------------------------------------------------------------
        MouseArea {
            id: miniMapInteraction
            anchors.fill: _pipView
            z: _pipView.z + 2
            visible: _pipView.visible && !_root._mainWindowIsMap
            enabled: visible
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            cursorShape: dragging ? Qt.ClosedHandCursor : Qt.OpenHandCursor

            property real lastX: 0
            property real lastY: 0
            property real pressX: 0
            property real pressY: 0
            property int pressButton: Qt.NoButton
            property bool dragging: false

            onPressed: function(mouse) {
                _root.prepareInteractiveMiniMap()
                pressX = mouse.x
                pressY = mouse.y
                lastX = mouse.x
                lastY = mouse.y
                pressButton = mouse.button
                dragging = false
            }

            onPositionChanged: function(mouse) {
                if (!pressed) {
                    return
                }

                if (!dragging &&
                    (Math.abs(mouse.x - pressX) > 5 || Math.abs(mouse.y - pressY) > 5)) {
                    dragging = true
                }

                if (dragging) {
                    var dx = mouse.x - lastX
                    var dy = mouse.y - lastY

                    if (pressButton === Qt.RightButton && _root._miniMap3D) {
                        _root.rotateMiniMap(dx, dy)
                    } else {
                        _root.panMiniMap(dx, dy)
                    }

                    lastX = mouse.x
                    lastY = mouse.y
                }
            }

            onReleased: function(mouse) {
                if (pressButton === Qt.LeftButton &&
                    !dragging &&
                    Math.abs(mouse.x - pressX) <= 5 &&
                    Math.abs(mouse.y - pressY) <= 5) {
                    _root.swapMapVideo()
                }
                pressButton = Qt.NoButton
                dragging = false
            }

            onCanceled: dragging = false

            onWheel: function(wheel) {
                _root.changeMapZoom(wheel.angleDelta.y > 0 ? 1 : -1)
                wheel.accepted = true
            }
        }

        // Quando o mapa está grande, a UI de payload sai da frente. Este
        // botão devolve imediatamente o vídeo à vista principal.
        Rectangle {
            id: returnToVideoButton
            z: QGroundControl.zOrderTopMost - 1
            visible: _root._mainWindowIsMap && QGroundControl.videoManager.hasVideo &&
                     !QGroundControl.videoManager.fullScreen
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: 14
            anchors.topMargin: toolbar.height + 12
            width: 132
            height: 42
            radius: 7
            color: returnVideoMouse.containsMouse ? "#2A4D62" : "#D00B1B26"
            border.width: 1
            border.color: "#4B7188"

            QGCLabel {
                anchors.centerIn: parent
                text: "← VIDEO"
                color: "white"
                font.bold: true
            }

            MouseArea {
                id: returnVideoMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: _root.swapMapVideo()
            }
        }

        // Expanded-map navigation: choose independent 2D/3D mode, reset
        // heading north, or recenter on the UAV. In 3D the native GeoMap
        // gestures remain available (pan/orbit/zoom).
        Row {
            id: fullMapControls
            z: QGroundControl.zOrderTopMost - 1
            visible: _root._mainWindowIsMap && !QGroundControl.videoManager.fullScreen
            anchors.left: QGroundControl.videoManager.hasVideo ? returnToVideoButton.right : parent.left
            anchors.top: parent.top
            anchors.leftMargin: 8
            anchors.topMargin: toolbar.height + 12
            spacing: 5

            Repeater {
                model: [
                    { "label": "2D", "action": "2D" },
                    { "label": "3D", "action": "3D" },
                    { "label": "N", "action": "NORTH" },
                    { "label": "CENTER UAV", "action": "CENTER" },
                    { "label": "KML", "action": "KML" }
                ]

                Rectangle {
                    width: modelData.action === "CENTER" ? 94 : (modelData.action === "KML" ? 58 : 42)
                    height: 42
                    radius: 7
                    color: modelData.action === "KML" && atlasKml.loaded && atlasKml.visible
                           ? "#A83232"
                           : (((modelData.action === "3D" && _root._fullMap3D) ||
                               (modelData.action === "2D" && !_root._fullMap3D))
                              ? "#249AF3" : "#D00B1B26")
                    border.width: 1
                    border.color: modelData.action === "KML" && atlasKml.loaded && atlasKml.visible
                                  ? "#FF6A6A"
                                  : (((modelData.action === "3D" && _root._fullMap3D) ||
                                      (modelData.action === "2D" && !_root._fullMap3D))
                                     ? "#62C4FF" : "#4B7188")

                    QGCLabel {
                        anchors.centerIn: parent
                        text: modelData.label
                        color: "white"
                        font.bold: true
                        font.pixelSize: 12
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (modelData.action === "3D") {
                                _root.setAtlasMap3D(true)
                            } else if (modelData.action === "2D") {
                                _root.setAtlasMap3D(false)
                            } else if (modelData.action === "NORTH") {
                                _root.resetMapNorth()
                            } else if (modelData.action === "KML") {
                                kmlPopup.open()
                            } else {
                                _root.centerMapOnVehicle()
                            }
                        }
                    }
                }
            }
        }

        FileDialog {
            id: kmlFileDialog
            title: "Import flight/restricted zones (.kml)"
            nameFilters: ["KML files (*.kml)"]
            fileMode: FileDialog.OpenFile
            onAccepted: {
                if (atlasKml.loadKml(selectedFile)) {
                    atlasKml.visible = true
                    Qt.callLater(_root.focusKmlZones)
                }
            }
        }

        Popup {
            id: kmlPopup
            parent: Overlay.overlay
            x: Math.max(12, (parent.width - width) / 2)
            y: Math.max(toolbar.height + 12, (parent.height - height) / 2)
            width: Math.min(780, parent.width - 24)
            height: Math.min(650, parent.height - toolbar.height - 24)
            modal: false
            focus: true
            padding: 16

            background: Rectangle {
                radius: 10
                color: "#F20A1720"
                border.width: 1
                border.color: "#3F6175"
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 9

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        QGCLabel {
                            text: "KML / AIRSPACE LAYERS"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 17
                        }

                        QGCLabel {
                            Layout.fillWidth: true
                            text: atlasKml.loaded
                                  ? atlasKml.fileName + "  •  " +
                                    atlasKml.folderCount + " folders  •  " +
                                    atlasKml.placemarkCount + " placemarks  •  " +
                                    atlasKml.featureCount + " geometries"
                                  : "No KML loaded"
                            color: "#B8CAD5"
                            elide: Text.ElideMiddle
                        }
                    }

                    QGCButton {
                        text: atlasKml.loaded ? "Import another" : "Import KML"
                        onClicked: kmlFileDialog.open()
                    }

                    QGCButton {
                        text: "Focus"
                        enabled: atlasKml.loaded
                        onClicked: _root.focusKmlZones()
                    }

                    QGCButton {
                        text: atlasKml.visible ? "MASTER ON" : "MASTER OFF"
                        enabled: atlasKml.loaded
                        onClicked: atlasKml.visible = !atlasKml.visible
                    }

                    QGCButton {
                        text: "Clear"
                        enabled: atlasKml.loaded
                        onClicked: atlasKml.clear()
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#355264"
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    QGCLabel {
                        Layout.fillWidth: true
                        text: "The tree follows the KML hierarchy. Folder and placemark switches control only their own layer; heights, extrusion and colours come from the file."
                        color: "#91A8B6"
                        font.pixelSize: 11
                        wrapMode: Text.WordWrap
                    }

                    QGCButton {
                        text: "All ON"
                        enabled: atlasKml.loaded
                        onClicked: atlasKml.setAllItemsVisible(true)
                    }

                    QGCButton {
                        text: "All OFF"
                        enabled: atlasKml.loaded
                        onClicked: atlasKml.setAllItemsVisible(false)
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 7
                    color: "#78050C11"
                    border.width: 1
                    border.color: "#294657"
                    clip: true

                    QGCLabel {
                        anchors.centerIn: parent
                        visible: !atlasKml.loaded
                        text: "Import a .kml file to display its layers."
                        color: "#8EA5B3"
                    }

                    ListView {
                        id: kmlTreeView
                        anchors.fill: parent
                        anchors.margins: 5
                        visible: atlasKml.loaded
                        model: atlasKml.treeItems
                        clip: true
                        spacing: 1

                        ScrollBar.vertical: ScrollBar {
                            policy: ScrollBar.AsNeeded
                        }

                        delegate: Rectangle {
                            width: kmlTreeView.width - 4
                            height: modelData.rowVisible ? 34 : 0
                            visible: modelData.rowVisible
                            radius: 4
                            color: treeMouse.containsMouse ? "#273B48" :
                                   (modelData.type === "folder" ? "#161F27" : "transparent")

                            Row {
                                anchors.fill: parent
                                anchors.leftMargin: 5 + modelData.depth * 16
                                anchors.rightMargin: 6
                                spacing: 5

                                Item {
                                    width: 18
                                    height: parent.height
                                    visible: modelData.type === "folder"

                                    QGCLabel {
                                        anchors.centerIn: parent
                                        text: modelData.childCount > 0
                                              ? (modelData.expanded ? "▾" : "▸")
                                              : ""
                                        color: "#B7D8E8"
                                        font.pixelSize: 14
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        enabled: modelData.childCount > 0
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            atlasKml.setTreeItemExpanded(
                                                modelData.id,
                                                !modelData.expanded)
                                        }
                                    }
                                }

                                Item {
                                    width: modelData.type === "folder" ? 0 : 18
                                    height: parent.height
                                    visible: modelData.type !== "folder"

                                    Rectangle {
                                        anchors.centerIn: parent
                                        width: 10
                                        height: 10
                                        radius: 2
                                        color: modelData.previewColor
                                        border.width: 1
                                        border.color: "#C8D6DE"
                                    }
                                }

                                CheckBox {
                                    anchors.verticalCenter: parent.verticalCenter
                                    checked: modelData.checked
                                    onClicked: {
                                        atlasKml.setTreeItemVisible(
                                            modelData.id,
                                            checked)
                                    }
                                }

                                Column {
                                    width: Math.max(
                                        120,
                                        parent.width - x - summaryColumn.width - 12)
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 0

                                    QGCLabel {
                                        width: parent.width
                                        text: modelData.name
                                        color: modelData.effectiveChecked
                                               ? "#E9F5FB"
                                               : "#728491"
                                        font.bold: modelData.type === "folder"
                                        font.pixelSize: 12
                                        elide: Text.ElideRight
                                    }

                                    QGCLabel {
                                        width: parent.width
                                        visible: modelData.type !== "folder" &&
                                                 modelData.summary.length > 0
                                        text: modelData.summary
                                        color: "#728C9B"
                                        font.pixelSize: 9
                                        elide: Text.ElideRight
                                    }
                                }

                                Column {
                                    id: summaryColumn
                                    width: 125
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 0

                                    QGCLabel {
                                        width: parent.width
                                        text: modelData.type === "folder"
                                              ? modelData.summary
                                              : modelData.altitude
                                        color: "#8EA8B7"
                                        font.pixelSize: 10
                                        horizontalAlignment: Text.AlignRight
                                        elide: Text.ElideLeft
                                    }
                                }
                            }

                            MouseArea {
                                id: treeMouse
                                anchors.fill: parent
                                z: -1
                                hoverEnabled: true
                                acceptedButtons: Qt.NoButton
                            }
                        }
                    }
                }

                QGCLabel {
                    Layout.fillWidth: true
                    visible: atlasKml.errorString.length > 0
                    wrapMode: Text.WordWrap
                    color: "#FF7A84"
                    text: atlasKml.errorString
                }

                QGCLabel {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: "#6F8796"
                    font.pixelSize: 10
                    text: "KML overlay is visual only. It does not upload a geofence to the Pixhawk."
                }
            }
        }

        // -----------------------------------------------------------------
        // ATLAS PAYLOAD UI
        // -----------------------------------------------------------------
        FlyViewCustomLayer {
            id: customOverlay

            anchors.fill: parent
            anchors.topMargin: toolbar.height

            z: QGroundControl.zOrderWidgets + 5
            parentToolInsets: payloadBaseInsets
            mapControl: _mapControl

            // Em Map Focus o mapa fica realmente livre para pan/zoom.
            visible: !QGroundControl.videoManager.fullScreen && !_root._mainWindowIsMap
        }

        GuidedActionsController {
            id: guidedActionsController
            missionController: _missionController
            guidedValueSlider: _guidedValueSlider
        }

        GuidedValueSlider {
            id: guidedValueSlider
            objectName: "guidedValueSlider"
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: toolbar.height
            anchors.bottom: parent.bottom
            z: QGroundControl.zOrderTopMost
            visible: false
        }

        Loader {
            id: viewer3DLoader
            z: 1
            anchors.fill: parent
            visible: _is3DMode
        }

        Connections {
            target: QGCViewer3DManager

            function onDisplayModeChanged() {
                if (QGCViewer3DManager.displayMode === QGCViewer3DManager.View3D) {
                    if (!viewer3DLoader.item) {
                        viewer3DLoader.setSource("qrc:/qml/QGroundControl/Viewer3D/Models3D/Viewer3DModel.qml")
                    }
                } else if (!_keepSceneAlive) {
                    viewer3DLoader.source = ""
                }
            }
        }
    }

    FlyViewToolBar {
        id: toolbar
        z: QGroundControl.zOrderTopMost
        guidedValueSlider: _guidedValueSlider
        visible: !QGroundControl.videoManager.fullScreen
    }
}

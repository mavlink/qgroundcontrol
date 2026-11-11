import QtQuick
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap
import QGroundControl.GeoMap

/// GeoMap that speaks the interface of the QtLocation FlightMap, so the views and controls written against
/// FlightMap (center, zoomLevel, toCoordinate/fromCoordinate, setVisibleRegion, isSatelliteMap, ...) work unchanged.
/// It also keeps the map view shared with the other views (QGroundControl.flightMapPosition/flightMapZoom) and can
/// show the camera controls and map scale itself. Views add their own content on top.
GeoMap {
    id: root

    // Writable like FlightMap's: assignments move the camera, camera moves are echoed back
    property var center
    property real zoomLevel: 0

    /// false: the zoom is neither restored from nor saved to the shared view, and survives resizes untouched.
    /// For a map that has its own zoom, such as a small picture-in-picture one.
    property bool sharedZoom: true

    /// Camera controls (2D/3D toggle and compass), optionally followed by the map scale, at the top left
    property bool showControls: false
    property bool showScale: false
    property real controlsLeftMargin: ScreenTools.defaultFontPixelWidth * 0.75
    property real controlsTopMargin: ScreenTools.defaultFontPixelWidth * 0.75

    readonly property bool isSatelliteMap: _mapTypeSetting.indexOf("Satellite") > -1 || _mapTypeSetting.indexOf("Hybrid") > -1

    readonly property string _mapTypeSetting: QGroundControl.settingsManager.flightMapSettings.mapType.rawValue

    // Breaks the center/zoomLevel <-> camera cycle: camera moves echo into them and must not be written back
    property bool _echoingCamera: false

    // Screen point -> coordinate of the rendered terrain surface under it, invalid on a sky pick.
    // clipToViewPort is accepted for FlightMap parity and ignored: every caller passes false.
    function toCoordinate(point, clipToViewPort) {
        return root.surfaceCoordinateAt(point)
    }

    // Coordinate -> screen point; an invalid or behind-the-camera coordinate projects off-screen.
    // clipToViewPort is ignored as in toCoordinate.
    function fromCoordinate(coordinate, clipToViewPort) {
        const screenPos = root.scene.screenPositionFor(coordinate)
        return (screenPos === undefined) ? Qt.point(-1, -1) : screenPos
    }

    // MapFitFunctions contract
    function setVisibleRegion(region) {
        root.fitToRegion(region)
    }

    // Zoom level to distance depends on the viewport size, so this also runs on every resize
    function _applyZoomLevel() {
        if (!_viewportValid()) {
            return
        }
        root.camera.distance = root.camera.distanceForZoomLevel(root.zoomLevel)
    }

    function _viewportValid() {
        const size = root.camera.viewportSize
        return size.width > 0 && size.height > 0
    }

    function _echoCameraCenter() {
        _echoingCamera = true
        center = root.camera.center
        _echoingCamera = false
    }

    function _echoCameraZoom() {
        if (!_viewportValid()) {
            return
        }
        _echoingCamera = true
        zoomLevel = root.camera.zoomLevelForDistance(root.camera.distance)
        _echoingCamera = false
    }

    function _restoreSharedView() {
        root.camera.center = QGroundControl.flightMapPosition
        if (sharedZoom) {
            zoomLevel = QGroundControl.flightMapZoom
        }
    }

    onCenterChanged: {
        if (_echoingCamera || !center || !center.isValid) {
            return
        }
        root.camera.center = center
    }

    onZoomLevelChanged: {
        if (!_echoingCamera) {
            _applyZoomLevel()
        }
    }

    onVisibleChanged: {
        if (visible) {
            _restoreSharedView()
        }
    }

    // Back from a map with its own zoom: the shared zoom applies again
    onSharedZoomChanged: {
        if (sharedZoom) {
            zoomLevel = QGroundControl.flightMapZoom
        }
    }

    Connections {
        target: root.camera

        function onCenterChanged() {
            root._echoCameraCenter()
            // Hidden: don't overwrite the position another view is sharing
            if (root.visible) {
                QGroundControl.flightMapPosition = root.camera.center
            }
        }

        function onDistanceChanged() {
            root._echoCameraZoom()
            if (root.visible && root.sharedZoom && root._viewportValid()) {
                QGroundControl.flightMapZoom = root.zoomLevel
            }
        }

        function onVerticalFieldOfViewChanged() {
            root._echoCameraZoom()
        }

        function onViewportSizeChanged() {
            if (root.sharedZoom) {
                root._applyZoomLevel()
            } else {
                root._echoCameraZoom()
            }
        }
    }

    Loader {
        x: root.controlsLeftMargin
        y: root.controlsTopMargin
        z: 1  // Above the content that views add, so a marker under a control does not take its clicks
        active: root.showControls || root.showScale
        visible: !ScreenTools.isTinyScreen

        sourceComponent: Row {
            spacing: ScreenTools.defaultFontPixelWidth * 0.75

            GeoMapControls {
                geoMap: root
                visible: root.showControls
            }

            MapScale {
                anchors.verticalCenter: parent.verticalCenter
                mapControl: root
                autoHide: true
                // The scale is ill-defined under the 3D tilt; isTopDown keeps it hidden until the 3D->2D tilt animation lands
                visible: root.showScale && root.camera.mode === GeoMapCamera.Mode2D && root.camera.isTopDown
            }
        }
    }

    Component.onCompleted: {
        _restoreSharedView()
        _echoCameraCenter()
        _echoCameraZoom()
    }
}

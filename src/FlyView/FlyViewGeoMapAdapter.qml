/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GeoMap

/// GeoMap-engine drop-in for FlyViewMap: hosts a FlyViewGeoMap and implements
/// the mapControl contract the Fly View overlays consume (pipState,
/// isSatelliteMap, toCoordinate/fromCoordinate, zoomLevel, ...).
Item {
    id: root

    // mapControl contract (parity with FlyViewMap)
    property Item pipView
    property Item pipState: _pipState
    property var rightPanelWidth
    property var planMasterController
    property bool pipMode: false // true: map is shown in a small pip mode
    property rect viewportRect: Qt.rect(0, 0, width, height)  // Map area the vehicle is kept within
    property var occluders: []                               // Rects within the viewport covered by UI
    property real chromeTopMargin: 0     // Map controls on the right edge start below this
    property real chromeBottomMargin: 0  // Map overlays on the bottom left stay above this
    property string mapName

    // Writable center (contract parity with FlyViewMap): assignments recentre
    // the camera; the camera Connections below keeps it tracking afterwards
    property var center
    readonly property real zoomLevel: {
        // Viewport/FOV referenced so the invokable re-evaluates on resize
        geoMapControl.camera.viewportSize
        geoMapControl.camera.verticalFieldOfView
        return geoMapControl.camera.zoomLevelForDistance(geoMapControl.camera.distance)
    }
    readonly property bool isSatelliteMap: _mapTypeSetting.indexOf("Satellite") > -1 || _mapTypeSetting.indexOf("Hybrid") > -1
    property alias pinchZoomDisabledByVirtualJoysticks: geoMapControl.pinchZoomDisabledByVirtualJoysticks

    // Escape hatch for GeoMap-specific chrome/controls
    readonly property var geoMap: geoMapControl

    readonly property string _mapTypeSetting: QGroundControl.settingsManager.flightMapSettings.mapType.rawValue

    // PiP analog of FlyViewMap._adjustMapZoomForPipMode: pull the camera back
    // 3 zoom levels in the small window for situational context; the swap-back
    // resize re-applies the shared saved zoom. PiP is also too small for a useful
    // 3D view, so force 2D and restore the previous camera mode with it.
    property int _fullViewCameraMode: GeoMapCamera.Mode2D

    onPipModeChanged: {
        const camera = geoMapControl.camera
        if (pipMode) {
            _fullViewCameraMode = camera.mode
            camera.mode = GeoMapCamera.Mode2D
            camera.distance = camera.distanceForZoomLevel(camera.zoomLevelForDistance(camera.distance) - 3)
        } else {
            camera.mode = _fullViewCameraMode
        }
    }

    // Map view shared with Plan view and persisted across restarts, as
    // FlyViewMap does via QGroundControl.flightMapPosition/flightMapZoom
    property bool _applyingSavedZoom: false

    function _restoreSavedMapView() {
        geoMapControl.camera.center = QGroundControl.flightMapPosition
        _applySavedZoom()
    }

    // Zoom level -> distance depends on the viewport size, so this also runs on
    // every resize: the zoom level is kept, as FlyViewMap does. PiP keeps its own zoom.
    function _applySavedZoom() {
        const camera = geoMapControl.camera
        if (pipMode || camera.viewportSize.width <= 0 || camera.viewportSize.height <= 0) {
            return
        }
        _applyingSavedZoom = true
        camera.distance = camera.distanceForZoomLevel(QGroundControl.flightMapZoom)
        _applyingSavedZoom = false
    }

    // Only the full-view zoom is saved: PiP distance changes are not
    function _onCameraDistanceChanged() {
        if (_applyingSavedZoom || !visible || pipMode) {
            return
        }
        const camera = geoMapControl.camera
        QGroundControl.flightMapZoom = camera.zoomLevelForDistance(camera.distance)
    }

    onVisibleChanged: {
        if (visible) {
            _restoreSavedMapView()
        }
    }

    // Breaks the center <-> camera.center cycle: camera moves echo into
    // center via the Connections below and must not be written back
    property bool _echoingCameraCenter: false

    onCenterChanged: {
        if (_echoingCameraCenter || !center || !center.isValid) {
            return
        }
        geoMapControl.camera.center = center
    }

    // Screen point -> coordinate of the rendered terrain surface under it
    // (clipToViewPort accepted for signature parity, screen points are
    // always inside the viewport for the fly view use cases)
    function toCoordinate(point, clipToViewPort) {
        return geoMapControl.surfaceCoordinateAt(point)
    }

    // Geographic coordinate -> screen point; invalid/behind-camera projects
    // to an off-screen point so distance math degrades instead of throwing
    function fromCoordinate(coordinate, clipToViewPort) {
        const screenPos = geoMapControl.scene.screenPositionFor(coordinate)
        return (screenPos === undefined) ? Qt.point(-1, -1) : screenPos
    }

    FlyViewGeoMap {
        id: geoMapControl
        anchors.fill: parent
        followViewportRect: root.viewportRect
        followOccluders: root.occluders

        // The PiP window is too small for free camera panning to be useful
        keepVehicleCentered: root.pipMode || QGroundControl.settingsManager.flyViewSettings.keepMapCenteredOnVehicle.rawValue

        // Guided-action popup on click (FlyViewMap.onMapClicked parity).
        // Allowed at any tilt: toCoordinate picks the rendered terrain
        // surface, so the click lands where the user sees it even in 3D.
        onMapClicked: (position) => {
            if (root.pipMode) {
                return
            }
            if (!globals.guidedControllerFlyView.guidedUIVisible &&
                (globals.guidedControllerFlyView.showGotoLocation || globals.guidedControllerFlyView.showOrbit ||
                 globals.guidedControllerFlyView.showROI || globals.guidedControllerFlyView.showSetHome ||
                 globals.guidedControllerFlyView.showSetEstimatorOrigin)) {

                const clickCoord = root.toCoordinate(Qt.point(position.x, position.y), false /* clipToViewPort */)
                if (!clickCoord.isValid) {
                    return
                }
                const windowPosition = root.mapToItem(globals.parent, position.x, position.y)
                const dropPanel = mapClickDropPanelComponent.createObject(mainWindow, { mapClickCoord: clickCoord, clickRect: Qt.rect(windowPosition.x, windowPosition.y, 0, 0) })
                dropPanel.open()
            }
        }
    }

    Component {
        id: mapClickDropPanelComponent

        FlyViewMapClickDropPanel {
            gotoIndicator: geoMapControl.gotoIndicator
            orbitIndicator: geoMapControl.orbitIndicator
        }
    }

    ObstacleDistanceOverlayMap {
        mapControl: root
        showText: !root.pipMode
    }

    FlyViewGeoMapChrome {
        anchors.fill: parent
        geoMap: geoMapControl
        visible: !root.pipMode
        buttonsTopMargin: root.chromeTopMargin
        overlayBottomMargin: root.chromeBottomMargin
    }

    PipState {
        id: _pipState
        pipView: root.pipView
        isDark: _isFullWindowItemDark
    }

    Connections {
        target: geoMapControl.camera

        function onCenterChanged() {
            root._echoingCameraCenter = true
            root.center = geoMapControl.camera.center
            root._echoingCameraCenter = false
            // Hidden: don't overwrite the position another view is sharing
            if (root.visible) {
                QGroundControl.flightMapPosition = geoMapControl.camera.center
            }
        }

        function onDistanceChanged() {
            root._onCameraDistanceChanged()
        }

        function onViewportSizeChanged() {
            root._applySavedZoom()
        }
    }

    Component.onCompleted: {
        _restoreSavedMapView()
        _echoingCameraCenter = true
        center = geoMapControl.camera.center
        _echoingCameraCenter = false
    }
}

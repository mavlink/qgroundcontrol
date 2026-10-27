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
import QGroundControl.FlightMap
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

    // The mapControl contract (center, zoomLevel, ...) is implemented by the map itself, see GeoMapView
    property alias center: geoMapControl.center
    property alias zoomLevel: geoMapControl.zoomLevel
    property alias isSatelliteMap: geoMapControl.isSatelliteMap
    property alias pinchZoomDisabledByVirtualJoysticks: geoMapControl.pinchZoomDisabledByVirtualJoysticks

    // Escape hatch for GeoMap-specific chrome/controls
    readonly property var geoMap: geoMapControl

    // PiP analog of FlyViewMap._adjustMapZoomForPipMode: pull the camera back
    // 3 zoom levels in the small window for situational context; the swap-back
    // re-applies the shared saved zoom. PiP is also too small for a useful
    // 3D view, so force 2D and restore the previous camera mode with it.
    property int _fullViewCameraMode: GeoMapCamera.Mode2D

    onPipModeChanged: {
        const camera = geoMapControl.camera
        // Pinned for the zoom change below, which must not be saved as the shared zoom, whichever
        // of this handler and the sharedZoom binding runs first
        geoMapControl.sharedZoom = !pipMode
        if (pipMode) {
            _fullViewCameraMode = camera.mode
            camera.mode = GeoMapCamera.Mode2D
            camera.distance = camera.distanceForZoomLevel(camera.zoomLevelForDistance(camera.distance) - 3)
        } else {
            camera.mode = _fullViewCameraMode
        }
        geoMapControl.sharedZoom = Qt.binding(() => !root.pipMode)
    }

    // MapFitFunctions contract (FlightMap.setVisibleRegion parity)
    function setVisibleRegion(region) {
        geoMapControl.setVisibleRegion(region)
        // PiP zoom is otherwise never shared: save the fit so the swap back keeps it (FlyViewMap parity)
        if (pipMode && visible) {
            const camera = geoMapControl.camera
            QGroundControl.flightMapZoom = camera.zoomLevelForDistance(camera.distance)
        }
    }

    function toCoordinate(point, clipToViewPort) {
        return geoMapControl.toCoordinate(point, clipToViewPort)
    }

    function fromCoordinate(coordinate, clipToViewPort) {
        return geoMapControl.fromCoordinate(coordinate, clipToViewPort)
    }

    FlyViewGeoMap {
        id: geoMapControl
        anchors.fill: parent
        followViewportRect: root.viewportRect
        followOccluders: root.occluders

        sharedZoom: !root.pipMode
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

    MapFitFunctions {
        id: mapFitFunctions
        map: root
        usePlannedHomePosition: false
        planMasterController: root.planMasterController
    }

    // Zoom to a mission downloaded from the vehicle (FlyViewMap parity)
    Connections {
        target: root.planMasterController ? root.planMasterController.missionController : null

        function onNewItemsFromVehicle() {
            const visualItems = root.planMasterController.missionController.visualItems
            if (visualItems && visualItems.count !== 1) {
                mapFitFunctions.fitMapViewportToMissionItems()
                geoMapControl.positionTracker.firstVehiclePositionReceived = true
            }
        }
    }
}

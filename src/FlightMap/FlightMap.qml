import QtQuick
import QtQuick.Controls
import QtLocation
import QtPositioning
import QtQuick.Dialogs

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap

Map {
    id: _map

    plugin:     QGCMapPlugin
    opacity:    0.99 // https://bugreports.qt.io/browse/QTBUG-82185

    // QtLocation's own notice is in the bottom left corner of the map, which the views cover: mapCredits is moved
    // clear of what covers it
    copyrightsVisible: false

    property string mapName:                        'defaultMap'
    // Also true for a map in dark colors: what is drawn on the map is light
    property bool   isSatelliteMap:                 QGroundControl.mapCatalog.isDarkMap(activeMapType.name)
    property var    gcsPosition:                    QGroundControl.positionManager.gcsPosition
    property real   gcsHeading:                     QGroundControl.positionManager.gcsHeading
    property alias  allowGCSLocationCenter:         _positionTracker.allowGCSLocationCenter     ///< true: map will center to gcs location one time
    property alias  allowVehicleLocationCenter:     _positionTracker.allowVehicleLocationCenter ///< true: map will center/zoom to vehicle location one time
    property alias  firstVehiclePositionReceived:   _positionTracker.firstVehiclePositionReceived ///< true: first vehicle position update was responded to
    property alias  positionTracker:                _positionTracker                            ///< Auto-centering policy, for derived maps to layer follow behavior on
    property bool   planView:                       false   ///< true: map being using for Plan view, items should be draggable
    property bool   pinchZoomDisabledByVirtualJoysticks: false ///< true: disable pinch-to-zoom while virtual joystick thumbs are down
    property real   creditsBottomMargin:            0       ///< Height of what covers the bottom left corner of the map, which the credits of the map are shown above

    property var    _activeVehicle:             QGroundControl.multiVehicleManager.activeVehicle
    property var    _activeVehicleCoordinate:   _activeVehicle ? _activeVehicle.coordinate : QtPositioning.coordinate()

    function setVisibleRegion(region) {
        // This works around a bug on Qt where if you set a visibleRegion and then the user moves or zooms the map
        // and then you set the same visibleRegion the map will not move/scale appropriately since it thinks there
        // is nothing to do.
        let maxZoomLevel = 20
        _map.visibleRegion = QtPositioning.rectangle(QtPositioning.coordinate(0, 0), QtPositioning.coordinate(0, 0))
        _map.visibleRegion = region
        if (_map.zoomLevel > maxZoomLevel) {
            _map.zoomLevel = maxZoomLevel
        }
    }

    /// Zooms in or out by \a delta zoom levels, no further in than the deepest zoom level of the map: QtLocation
    /// lets the zoom level go on to 30, where a map has nothing to show
    function zoomBy(delta) {
        _map.zoomLevel = Math.min(Math.max(_map.zoomLevel + delta, 0), _map.maximumZoomLevel)
    }

    // A map without its plugin shows nothing, and says nothing else of it
    onErrorChanged: {
        if (error !== Map.NoError) {
            console.warn("The map", mapName, "cannot show tiles:", errorString)
        }
    }

    function centerToSpecifiedLocation() {
        specifyMapPositionDialogFactory.open()
    }

    MapCopyrightNotice {
        id:                     mapCredits
        objectName:             "mapCredits"
        mapSource:              _map
        anchors.left:           parent.left
        anchors.bottom:         parent.bottom
        anchors.bottomMargin:   _map.creditsBottomMargin
        z:                      QGroundControl.zOrderMapItems
    }

    MapPositionTracker {
        id: _positionTracker

        active: _map.mapReady
        gcsPosition: _map.gcsPosition
        vehicleCoordinate: _map._activeVehicleCoordinate
        centerGCSWhenVehicleValid: QGroundControl.settingsManager.flyViewSettings.keepMapCenteredOnVehicle.rawValue

        onCenterMap: (coordinate, firstPosition) => {
            _map.center = coordinate
            if (firstPosition) {
                _map.zoomLevel = QGroundControl.flightMapInitialZoom
            }
        }
    }

    QGCPopupDialogFactory {
        id: specifyMapPositionDialogFactory

        dialogComponent: specifyMapPositionDialog
    }

    Component {
        id: specifyMapPositionDialog
        EditPositionDialog {
            title:                  qsTr("Specify Position")
            coordinate:             center
            onCoordinateChanged:    center = coordinate
        }
    }

    function updateActiveMapType() {
        var chosenMap = QGroundControl.mapCatalog.chosenMap

        for (var i = 0; i < _map.supportedMapTypes.length; i++) {
            if (chosenMap === _map.supportedMapTypes[i].name) {
                _map.activeMapType = _map.supportedMapTypes[i]
                return
            }
        }
    }

    onMapReadyChanged: {
        if (_map.mapReady) {
            updateActiveMapType()
        }
    }

    onVisibleRegionChanged: prefetchTimer.restart()

    Connections {
        target: QGroundControl.mapCatalog
        function onChosenMapChanged() { updateActiveMapType() }
    }

    // QtLocation fetches the tiles around the view, which the next pan or zoom shows, only when it is asked to.
    // It is asked once the view has come to rest: a request for a tile ends when the view moves on.
    Timer {
        id:         prefetchTimer
        interval:   500

        onTriggered: {
            if (_map.visible) {
                _map.prefetchData()
            }
        }
    }

    signal mapPanStart
    signal mapPanStop
    signal mapClicked(var position)
    signal mapRightClicked(var position)
    signal mapPressAndHold(var position)

    PinchHandler {
        id:      pinchHandler
        target:  null
        // Disabled while virtual joystick thumbs are down. The default grabPermissions include
        // CanTakeOverFromItems, which lets this handler steal touch grabs from the joystick pads'
        // MultiPointTouchAreas and zoom the map instead. The permission can't be removed since
        // pinch on the map itself must take over from the panning MultiPointTouchArea below.
        // See GitHub issue #13450.
        enabled: !_map.pinchZoomDisabledByVirtualJoysticks

        property var pinchStartGeoCoord     // geo coordinate under centroid at pinch start
        property var pinchStartScreenPoint  // screen point of centroid at pinch start

        onActiveChanged: {
            if (active) {
                // Capture both the screen point and its geo coordinate once at pinch start.
                // alignCoordinateToPoint requires a fixed screen anchor; using the live
                // centroid.position causes the map to pan as fingers drift.
                pinchStartScreenPoint = pinchHandler.centroid.position
                pinchStartGeoCoord    = _map.toCoordinate(pinchStartScreenPoint, false)
            }
        }
        onScaleChanged: (delta) => {
            _map.zoomBy(Math.log2(delta))
            _map.alignCoordinateToPoint(pinchStartGeoCoord, pinchStartScreenPoint)
        }
    }

    WheelHandler {
        // WheelHandler's default acceptedDevices=Mouse silently drops trackpad scroll events on
        // multiple platforms:
        //   - Linux/Wayland (QTBUG-112394 / QTBUG-112432): the Wayland
        //     protocol exposes no way to distinguish a mouse from a trackpad, so Qt registers all
        //     pointer devices as TouchPad.
        //   - xcb / XWayland: Wayland pointer events are translated back to X11 and device-type
        //     metadata is lost — physical mouse scroll events arrive as PointerDevice.TouchPad.
        //   - macOS (cocoa): trackpad scroll events are correctly reported as PointerDevice.TouchPad
        //     but are excluded by the Mouse-only default.
        // Accepting both Mouse and TouchPad on all platforms is harmless and covers every case.
        acceptedDevices:    PointerDevice.Mouse | PointerDevice.TouchPad
        rotationScale:      1 / 120

        onWheel: (event) => {
            const zoomDelta = event.angleDelta.y * rotationScale
            const mouseGeoPos = _map.toCoordinate(Qt.point(event.x, event.y), false)
            _map.zoomBy(zoomDelta)
            _map.alignCoordinateToPoint(mouseGeoPos, Qt.point(event.x, event.y))
        }
    }

    // We specifically do not use a DragHandler for panning. It just causes too many problems if you overlay anything else like a Flickable above it.
    // Causes all sorts of crazy problems where dragging/scrolling  no longerr works on items above in the hierarchy.
    // Since we are using a MouseArea we also can't use TapHandler for clicks. So we handle that here as well.
    MultiPointTouchArea {
        id: multiTouchArea
        anchors.fill: parent
        maximumTouchPoints: 1
        mouseEnabled: true

        property bool dragActive: false
        property real lastMouseX
        property real lastMouseY
        property bool isPressed: false
        property bool pressAndHold: false

        onPressed: (touchPoints) => {
            lastMouseX = touchPoints[0].x
            lastMouseY = touchPoints[0].y
            isPressed = true
            pressAndHold = false
            pressAndHoldTimer.start()
        }

        onGestureStarted: (gesture) => {
            dragActive = true
            gesture.grab()
            mapPanStart()
        }

        onUpdated: (touchPoints) => {
            if (dragActive) {
                let deltaX = touchPoints[0].x - lastMouseX
                let deltaY = touchPoints[0].y - lastMouseY
                if (Math.abs(deltaX) >= 1.0 || Math.abs(deltaY) >= 1.0) {
                    _map.pan(lastMouseX - touchPoints[0].x, lastMouseY - touchPoints[0].y)
                    lastMouseX = touchPoints[0].x
                    lastMouseY = touchPoints[0].y
                }
            }
        }

        onReleased: (touchPoints) => {
            isPressed = false
            pressAndHoldTimer.stop()
            if (dragActive) {
                _map.pan(lastMouseX - touchPoints[0].x, lastMouseY - touchPoints[0].y)
                dragActive = false
                mapPanStop()
            } else if (!pressAndHold) {
                mapClicked(Qt.point(touchPoints[0].x, touchPoints[0].y))
            }
            pressAndHold = false
        }

        Timer {
            id: pressAndHoldTimer
            interval: 600        // hold duration in ms
            repeat: false

            onTriggered: {
                if (multiTouchArea.isPressed && !multiTouchArea.dragActive) {
                    multiTouchArea.pressAndHold = true
                    mapPressAndHold(Qt.point(multiTouchArea.lastMouseX, multiTouchArea.lastMouseY))
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        propagateComposedEvents: true

        onPressed: (mouseEvent) => {
            if (mouseEvent.button === Qt.RightButton) {
                mapRightClicked(Qt.point(mouseEvent.x, mouseEvent.y))
            }
        }
    }

    /// Ground Station location
    MapQuickItem {
        anchorPoint.x:  sourceItem.width / 2
        anchorPoint.y:  sourceItem.height / 2
        visible:        gcsPosition.isValid && !planView
        coordinate:     gcsPosition

        sourceItem: Image {
            id:             mapItemImage
            source:         isNaN(gcsHeading) ? "/res/QGCLogoFull.svg" : "/res/QGCLogoArrow.svg"
            mipmap:         true
            antialiasing:   true
            fillMode:       Image.PreserveAspectFit
            height:         ScreenTools.defaultFontPixelHeight * (isNaN(gcsHeading) ? 1.75 : 2.5 )
            sourceSize.height: height
            transform: Rotation {
                origin.x:       mapItemImage.width  / 2
                origin.y:       mapItemImage.height / 2
                angle:          isNaN(gcsHeading) ? 0 : gcsHeading
            }
        }
    }
} // Map

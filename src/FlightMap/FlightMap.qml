/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtLocation
import QtPositioning
import QtQuick.Dialogs
import Qt.labs.animation

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap

Map {
    id: _map

    plugin:     Plugin { name: "QGroundControl" }
    opacity:    0.99 // https://bugreports.qt.io/browse/QTBUG-82185

    property string mapName:                        'defaultMap'
    property bool   isSatelliteMap:                 activeMapType.name.indexOf("Satellite") > -1 || activeMapType.name.indexOf("Hybrid") > -1
    property var    gcsPosition:                    QGroundControl.qgcPositionManger.gcsPosition
    property real   gcsHeading:                     QGroundControl.qgcPositionManger.gcsHeading
    property bool   allowGCSLocationCenter:         false   ///< true: map will center/zoom to gcs location one time
    property bool   allowVehicleLocationCenter:     false   ///< true: map will center/zoom to vehicle location one time
    property bool   firstGCSPositionReceived:       false   ///< true: first gcs position update was responded to
    property bool   firstVehiclePositionReceived:   false   ///< true: first vehicle position update was responded to
    property bool   planView:                       false   ///< true: map being using for Plan view, items should be draggable

    property var    _activeVehicle:             QGroundControl.multiVehicleManager.activeVehicle
    property var    _activeVehicleCoordinate:   _activeVehicle ? _activeVehicle.coordinate : QtPositioning.coordinate()
    property var    _kamikazeLocManager:        QGroundControl.kamikazeLocManager

    // KEYBOARD map controls
    focus: true
    property bool wPressed: false
    property bool aPressed: false
    property bool sPressed: false
    property bool dPressed: false
    property bool qPressed: false
    property bool ePressed: false

    Timer {
        id: panTimer
        interval: 16
        running: true
        repeat: true
        onTriggered: {
            const basePan = 0.0001
            const zoomFactor = Math.pow(2, 18 - _map.zoomLevel)  // tweak 18 to your max zoom
            const panAmount = basePan * zoomFactor

            let dx = 0, dy = 0
            if (wPressed) dy += 1
            if (sPressed) dy -= 1
            if (aPressed) dx -= 1
            if (dPressed) dx += 1

            const length = Math.sqrt(dx*dx + dy*dy)
            if (length > 0) {
                _map.center.latitude += (dy / length) * panAmount
                _map.center.longitude += (dx / length) * panAmount
            }

            if (qPressed) _map.zoomLevel = Math.max(_map.zoomLevel - 0.1, 2) // Minimum zoom level
            if (ePressed) _map.zoomLevel = Math.min(_map.zoomLevel + 0.1, 18) // Maximum zoom level

        }
    }

    Keys.onPressed: function(event) {
        switch (event.key) {
        case Qt.Key_W: wPressed = true; break;
        case Qt.Key_A: aPressed = true; break;
        case Qt.Key_S: sPressed = true; break;
        case Qt.Key_D: dPressed = true; break;
        case Qt.Key_Q: qPressed = true; break;
        case Qt.Key_E: ePressed = true; break;
        }
    }

    Keys.onReleased: function(event) {
        switch (event.key) {
        case Qt.Key_W: wPressed = false; break;
        case Qt.Key_A: aPressed = false; break;
        case Qt.Key_S: sPressed = false; break;
        case Qt.Key_D: dPressed = false; break;
        case Qt.Key_Q: qPressed = false; break;
        case Qt.Key_E: ePressed = false; break;
        }
    }

    // QR icon at set location
    MapQuickItem {
        anchorPoint.x: kamikaze_icon.width / 2
        anchorPoint.y: kamikaze_icon.height / 2
        visible: true
        coordinate: _kamikazeLocManager.coordinate

        sourceItem: Image {
            id: kamikaze_icon
            source:  "/res/qr.png" //"/res/zoom-gps.svg"
            mipmap: true
            antialiasing: true
            fillMode: Image.PreserveAspectFit

            property real baseSize: ScreenTools.defaultFontPixelHeight / 8
            property real referenceZoom: 15

            // SCALE WITH ZOOM
            width:  baseSize * Math.pow(2, _map.zoomLevel - referenceZoom)
            height: width

            sourceSize.height: height
            transform: Rotation {
                origin.x:       kamikaze_icon.width  / 2
                origin.y:       kamikaze_icon.height / 2
                angle:          0
            }
        }
    }

    function setVisibleRegion(region) {
        // TODO: Is this still necessary with Qt 5.11?
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

    function _possiblyCenterToVehiclePosition() {
        if (!firstVehiclePositionReceived && allowVehicleLocationCenter && _activeVehicleCoordinate.isValid) {
            firstVehiclePositionReceived = true
            center = _activeVehicleCoordinate
            zoomLevel = QGroundControl.flightMapInitialZoom
        }
    }

    function centerToSpecifiedLocation() {
        specifyMapPositionDialog.createObject(mainWindow).open()
    }

    Component {
        id: specifyMapPositionDialog
        EditPositionDialog {
            title:                  qsTr("Specify Position")
            coordinate:             center
            onCoordinateChanged:    center = coordinate
        }
    }

    // Center map to gcs location
    onGcsPositionChanged: {
        if (gcsPosition.isValid && allowGCSLocationCenter && !firstGCSPositionReceived && !firstVehiclePositionReceived) {
            firstGCSPositionReceived = true
            //-- Only center on gsc if we have no vehicle (and we are supposed to do so)
            var _activeVehicleCoordinate = _activeVehicle ? _activeVehicle.coordinate : QtPositioning.coordinate()
            if(QGroundControl.settingsManager.flyViewSettings.keepMapCenteredOnVehicle.rawValue || !_activeVehicleCoordinate.isValid)
                center = gcsPosition
        }
    }

    function updateActiveMapType() {
        var settings =  QGroundControl.settingsManager.flightMapSettings
        var fullMapName = settings.mapProvider.value + " " + settings.mapType.value

        for (var i = 0; i < _map.supportedMapTypes.length; i++) {
            if (fullMapName === _map.supportedMapTypes[i].name) {
                _map.activeMapType = _map.supportedMapTypes[i]
                return
            }
        }
    }

    on_ActiveVehicleCoordinateChanged: _possiblyCenterToVehiclePosition()

    onMapReadyChanged: {
        if (_map.mapReady) {
            updateActiveMapType()
            _possiblyCenterToVehiclePosition()
        }
    }

    Connections {
        target: QGroundControl.settingsManager.flightMapSettings.mapType
        function onRawValueChanged() { updateActiveMapType() }
    }

    Connections {
        target: QGroundControl.settingsManager.flightMapSettings.mapProvider
        function onRawValueChanged() { updateActiveMapType() }
    }

    signal mapPanStart
    signal mapPanStop
    signal mapClicked(var position)
    signal mapRightClicked(var position)
    signal mapPressAndHold(var position)

    PinchHandler {
        id:     pinchHandler
        target: null

        property var pinchStartCentroid

        onActiveChanged: {
            if (active) {
                pinchStartCentroid = _map.toCoordinate(pinchHandler.centroid.position, false)
            }
        }
        onScaleChanged: (delta) => {
            let newZoomLevel = Math.max(_map.zoomLevel + Math.log2(delta), 0)
            _map.zoomLevel = newZoomLevel
            _map.alignCoordinateToPoint(pinchStartCentroid, pinchHandler.centroid.position)
        }
    }

    WheelHandler {
        // workaround for QTBUG-87646 / QTBUG-112394 / QTBUG-112432:
        // Magic Mouse pretends to be a trackpad but doesn't work with PinchHandler
        // and we don't yet distinguish mice and trackpads on Wayland either
        acceptedDevices:    Qt.platform.pluginName === "cocoa" || Qt.platform.pluginName === "wayland" ?
                                PointerDevice.Mouse | PointerDevice.TouchPad : PointerDevice.Mouse
        rotationScale:      1 / 120
        property:           "zoomLevel"

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

            let coord = _map.toCoordinate(Qt.point(touchPoints[0].x, touchPoints[0].y), false)
            if (_kamikazeLocManager) {
                _kamikazeLocManager.setCoordinate(coord)
            } else {
                console.warn("kamikazeLocManager not available yet")
            }
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
        visible:        gcsPosition.isValid
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

    // restore Focus FIX: for changing page loses focus, WASD movement not working
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton

        onPressed: function(mouse) {
            forceActiveFocus() // restore keyboard focus to the Map
            mouse.accepted = false // IMPORTANT: allow the event to propagate so underlying handlers still run
        }

        onEntered: function() { forceActiveFocus() }
    }


    PositionSource {
        id: deviceLocation
        active: true
        preferredPositioningMethods: PositionSource.SatellitePositioningMethods | PositionSource.NonSatellitePositioningMethods
    }

    QGCButton {
        id: zoomToDeviceButton
        icon.source: "qrc:/res/zoom-gps.svg"
        icon.color: "green"
        anchors {
            left: parent.left
            bottom: parent.bottom
            leftMargin: ScreenTools.defaultFontPixelWidth * 2
            bottomMargin: ScreenTools.defaultFontPixelHeight * 2
        }
        ToolTip.text: qsTr("Zoom to My GPS Location")
        visible: true

        onClicked: {
            if (deviceLocation.position.coordinate.isValid) {
                _map.center = deviceLocation.position.coordinate
                _map.zoomLevel = 18
            } else {
                console.warn("Device location not available; using IP fallback")
                // Example: city-level fallback
                _map.center = QtPositioning.coordinate(39.815565, 30.531929)
                _map.zoomLevel = 15
            }
        }
    }

} // Map

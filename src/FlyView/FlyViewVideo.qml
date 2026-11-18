import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.VideoManager

Item {
    id: root

    property Item pipView
    /// Height the Fly View toolbar covers at the top of the full-size video.
    property real topInset: 0
    /// Width the Fly View's widgets cover at the top right of the full-size video.
    property real rightInset: 0
    property Item pipState: videoPipState
    /// The manager whose video the view shows.
    property VideoManager videoManager: QGroundControl.videoManager

    PipState {
        id:         videoPipState
        pipView:    root.pipView
        isDark:     true

        onStateChanged: {
            if (videoPipState.state !== videoPipState.fullState) {
                root.videoManager.fullScreen = false
            }
        }
    }

    // Under the video view, whose tiles take their touches first. With one finger down as the mouse area's press, a
    // second one reaches it: the two pinch.
    CameraPinchZoom {
        anchors.fill:   parent
        camera:         videoStreaming.camera
        heldFinger:     Qt.point(flyViewVideoMouseArea.mouseX, flyViewVideoMouseArea.mouseY)
        heldFingerDown: flyViewVideoMouseArea.pressed

        onPinchingChanged: {
            if (pinching) {
                flyViewVideoMouseArea.pinchBegan()
            }
        }
    }

    //-- Video Streaming
    VideoView {
        id:                 videoStreaming
        anchors.fill:       parent
        useSmallFont:       root.pipState.state !== root.pipState.fullState
        thermalPipTopInset: root.pipState.state === root.pipState.fullState ? root.topInset : 0
        showTiles:          root.pipState.state === root.pipState.fullState
        tilesRightInset:    root.rightInset
        videoManager:       root.videoManager
        visible:        root.videoManager.isStreamSource || root.videoManager.isUvc
    }

    QGCLabel {
        id: fullScreenHint

        text: qsTr("Double-click to exit full screen")
        font.pointSize: ScreenTools.largeFontPointSize
        visible: root.videoManager.fullScreen
        anchors.centerIn: parent

        // Starts over each time the hint is shown. Left to itself it runs once, from when the view was created.
        PropertyAnimation on opacity {
            running: fullScreenHint.visible
            duration: 10000
            from: 1.0
            to: 0.0
            easing.type: Easing.InExpo
        }
    }

    OnScreenGimbalController {
        id:                      onScreenGimbalController
        anchors.fill:            parent
        cameraTrackingEnabled:   !!(videoStreaming.camera && videoStreaming.camera.trackingEnabled)
    }

    OnScreenCameraTrackingController {
        id:                      cameraTrackingController
        anchors.fill:            parent
        camera:                  videoStreaming.camera
        videoWidth:              videoStreaming.videoWidth
        videoHeight:             videoStreaming.videoHeight
    }

    MouseArea {
        id:                         flyViewVideoMouseArea

        property real _pressX:      0
        property real _pressY:      0
        property bool _dragging:    false
        property bool _doubleClicked: false
        // A second finger made the press a pinch, which is neither a click nor a drag.
        property bool _pinched: false
        readonly property real _dragThreshold: 10

        function pinchBegan() {
            _pinched = true
            if (_dragging) {
                // The drag ends without tracking what it spanned, which a rectangle without a size doesn't.
                onScreenGimbalController.mouseDragEnd()
                cameraTrackingController.mouseDragEnd(_pressX, _pressY)
                _dragging = false
            }
        }

        objectName:                 "flyViewVideoMouseArea"
        anchors.fill:               parent
        enabled:                    root.pipState.state === root.pipState.fullState

        // Defer single-click handling so a double-click (fullscreen toggle) doesn't also
        // fire an unintended gimbal click-to-point/tracking command on its first click.
        Timer {
            id:         singleClickTimer
            objectName: "flyViewVideoClickTimer"

            property real clickX: 0
            property real clickY: 0

            interval:   Qt.styleHints.mouseDoubleClickInterval
            repeat:     false

            onTriggered: {
                onScreenGimbalController.mouseClicked(clickX, clickY)
                cameraTrackingController.mouseClicked(clickX, clickY)
            }
        }

        onDoubleClicked: (mouse) => {
            if (videoStreaming.tileAt(mouse.x, mouse.y)) {
                mouse.accepted = false
                return
            }
            // Fires on the second press of a double-click. The second release still emits
            // onReleased, so flag it to prevent re-arming the single-click timer.
            _doubleClicked = true
            singleClickTimer.stop()
            root.videoManager.fullScreen = !root.videoManager.fullScreen
        }

        onPressed: (mouse) => {
            // A press on a video tile is the tile's, which is below this area.
            if (videoStreaming.tileAt(mouse.x, mouse.y)) {
                mouse.accepted = false
                return
            }
            _pressX = mouse.x
            _pressY = mouse.y
            _dragging = false
            _pinched = false
            // Clear any stale flag (e.g. double-click followed by drag releases through the
            // drag branch without consuming it). Safe: pressed is emitted before doubleClicked.
            _doubleClicked = false
        }

        onPositionChanged: (mouse) => {
            if (_pinched) {
                return
            }
            if (!_dragging && (Math.abs(mouse.x - _pressX) >= _dragThreshold || Math.abs(mouse.y - _pressY) >= _dragThreshold)) {
                _dragging = true
                onScreenGimbalController.mouseDragStart(_pressX, _pressY)
                cameraTrackingController.mouseDragStart(_pressX, _pressY)
            }
            if (_dragging) {
                onScreenGimbalController.mouseDragPositionChanged(mouse.x, mouse.y)
                cameraTrackingController.mouseDragPositionChanged(mouse.x, mouse.y)
            }
        }

        onReleased: (mouse) => {
            if (_pinched) {
                return
            }
            if (_dragging) {
                onScreenGimbalController.mouseDragEnd()
                cameraTrackingController.mouseDragEnd(mouse.x, mouse.y)
            } else if (_doubleClicked) {
                // Second release of a double-click - fullscreen toggle already handled
                _doubleClicked = false
            } else {
                singleClickTimer.clickX = mouse.x
                singleClickTimer.clickY = mouse.y
                singleClickTimer.restart()
            }
            _dragging = false
        }
    }

    ProximityRadarVideoView{
        anchors.fill:   parent
        vehicle:        QGroundControl.multiVehicleManager.activeVehicle
    }

    ObstacleDistanceOverlayVideo {
        id: obstacleDistance
        showText: root.pipState.state === root.pipState.fullState
    }
}

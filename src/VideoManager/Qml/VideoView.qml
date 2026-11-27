pragma ComponentBehavior: Bound

import QtQuick

import QGroundControl
import QGroundControl.Controls

// The video of the primary stream or the UVC camera, fitted to the view, with grid lines, the telemetry overlay and the
// thermal stream. Where it is asked to, it shows the other video sources in tiles along its top, and takes the presses
// on the video: it reports clicks and drags, and toggles full screen on a double-click. Two fingers zoom the camera.
Item {
    id: root

    property bool useSmallFont: true
    /// Height covered at the top of the view, which the thermal picture-in-picture and the tiles stay below.
    property real thermalPipTopInset: 0
    /// Whether the other video sources are shown in tiles: for a view with room for them, not a picture-in-picture.
    property bool showTiles: false
    /// Width covered at the right of the view, which the tiles stay clear of.
    property real tilesRightInset: 0
    /// The camera whose thermal mode the view follows and whose zoom two fingers step: the active vehicle's current
    /// camera by default. Any object with the camera properties the view reads will do, as the tests' stand-ins.
    property var camera: root._activeVehicle && root._activeVehicle.cameraManager
                         ? root._activeVehicle.cameraManager.currentCameraInstance : null
    /// The manager whose video the view shows.
    property VideoManager videoManager: QGroundControl.videoManager
    /// Whether the video is shown: the picture, or why there is none, and the tiles. Presses and pinches are taken
    /// either way.
    property bool showVideo: true
    /// Whether the view takes the presses on the video, for clicks, drags and double-clicks. A press on a tile is the
    /// tile's.
    property bool interactive: false

    /// Size of the video fitted to the view: larger than the view where the fit mode crops, smaller where it doesn't.
    readonly property real videoWidth: {
        if (root._ar > 0) {
            if (root._isModeFitHeight
                    || (root._isModeFill && (root.width / root.height < root._ar))
                    || (root._isModeNoCrop && (root.width / root.height > root._ar))) {
                // This return value has different implications depending on the mode
                // For FIT_HEIGHT and FILL
                //    makes so the video width will be larger than (or equal to) the screen width
                // For NO_CROP Mode
                //    makes so the video width will be smaller than (or equal to) the screen width
                return root.height * root._ar
            }
        }
        return root.width
    }
    readonly property real videoHeight: {
        if (root._ar > 0) {
            if (root._isModeFitWidth
                    || (root._isModeFill && (root.width / root.height > root._ar))
                    || (root._isModeNoCrop && (root.width / root.height < root._ar))) {
                // This return value has different implications depending on the mode
                // For FIT_WIDTH and FILL
                //    makes so the video height will be larger than (or equal to) the screen height
                // For NO_CROP Mode
                //    makes so the video height will be smaller than (or equal to) the screen height
                return root.width * (1 / root._ar)
            }
        }
        return root.height
    }

    readonly property VideoSettings _videoSettings: QGroundControl.settingsManager.videoSettings
    readonly property Vehicle _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    readonly property VideoStreamController _mainSource: root.videoManager.mainSource
    // A lost stream's last frame stays up, under a notice, while it reconnects.
    readonly property bool _streamLost: root._mainSource.reconnecting
    readonly property bool _hasPicture: root._mainSource.decoding || root._streamLost
    // The ratio of the picture shown: the one forced for the stream, else the video's own, known once its first frame
    // sets the output's implicit size. Until then the video fills the view.
    readonly property real _ar: mainOutput.aspectRatio
    readonly property int _fitMode: root._videoSettings.videoFit.rawValue
    readonly property bool _isModeFitWidth: root._fitMode === VideoSettings.VideoFitWidth
    readonly property bool _isModeFitHeight: root._fitMode === VideoSettings.VideoFitHeight
    readonly property bool _isModeFill: root._fitMode === VideoSettings.VideoFitFill
    readonly property bool _isModeNoCrop: root._fitMode === VideoSettings.VideoFitNoCrop
    // The output fills the view, and crops the video to it where the fitted video is larger.
    readonly property real _viewAr: root.width / root.height
    readonly property bool _cropsVideo: root._isModeFill
                                        || ((root._ar > 0)
                                            && ((root._isModeFitWidth && (root._viewAr > root._ar))
                                                || (root._isModeFitHeight && (root._viewAr < root._ar))))
    readonly property real _tilesMargin: ScreenTools.defaultFontPixelHeight * 0.5
    readonly property bool _thermalPipShown: thermalOverlay.visible && !!root.camera
                                             && root.camera.thermalMode === MavlinkCameraControlInterface.THERMAL_PIP

    /// A click at @p x, @p y, reported once the double-click interval has passed without a second click.
    signal clicked(real x, real y)
    /// A drag began at @p x, @p y, where it was pressed.
    signal dragStarted(real x, real y)
    /// The drag is at @p x, @p y.
    signal dragMoved(real x, real y)
    /// The drag ended at @p x, @p y: where it was released, or where it began once a second finger made it a pinch.
    signal dragEnded(real x, real y)

    /// Whether a tile is at @p x, @p y of the view: a press there is the tile's.
    function tileAt(x, y) {
        return tiles.visible && !!tiles.childAt(x - tiles.x, y - tiles.y)
    }

    // The grid lines span the fitted video, beyond the view where it crops.
    clip: true

    // Under the tiles, which take their touches first, and under the mouse area. With one finger down as the mouse
    // area's press, a second one reaches it: the two pinch.
    CameraPinchZoom {
        id:             pinchZoom
        anchors.fill:   parent
        camera:         root.camera
        heldFinger:     Qt.point(mouseArea.mouseX, mouseArea.mouseY)
        heldFingerDown: mouseArea.pressed

        onPinchingChanged: {
            if (pinchZoom.pinching) {
                mouseArea.pinchBegan()
            }
        }
    }

    // The video alone is hidden: a hidden item loses the presses it has, which the mouse area and the pinch keep.
    Item {
        anchors.fill:   parent
        visible:        root.showVideo

        NoVideoPlaceholder {
            anchors.fill:   parent
            useSmallFont:   root.useSmallFont
            videoManager:   root.videoManager
            visible:        !root._hasPicture
        }

        Rectangle {
            anchors.fill:   parent
            color:          "black"
            visible:        root._hasPicture

            VideoStreamOutput {
                id:             mainOutput
                objectName:     "videoViewOutput"
                anchors.fill:   parent
                stream:         root._mainSource
                crop:           root._cropsVideo
            }

            Item {
                id:                 videoContentArea
                width:              root.videoWidth
                height:             root.videoHeight
                anchors.centerIn:   parent

                VideoGridLines {
                    anchors.fill:   parent
                    visible:        root._videoSettings.gridLines.rawValue && !root.videoManager.fullScreen
                }
            }

            // The part of the fitted video that is on screen.
            Item {
                width:              Math.min(root.videoWidth, root.width)
                height:             Math.min(root.videoHeight, root.height)
                anchors.centerIn:   parent

                TelemetryOverlayText {
                    objectName:     "videoTelemetryOverlay"
                    text:           root.videoManager.telemetryOverlayText
                    useSmallFont:   root.useSmallFont
                }
            }

            // Frozen video must not pass for live.
            VideoStatusLabel {
                anchors.centerIn:   parent
                useSmallFont:       root.useSmallFont
                text:               qsTr("VIDEO LOST - RECONNECTING")
                visible:            root._streamLost
            }

            ThermalVideoOverlay {
                id:             thermalOverlay
                objectName:     "videoViewThermal"
                camera:         root.camera
                pipTopInset:    root.thermalPipTopInset
                videoManager:   root.videoManager
            }
        }

        // Along the top, right of the thermal picture-in-picture, or in its place: clear of the Fly View's tool strip,
        // as that one is.
        VideoTiles {
            id:             tiles
            objectName:     "videoViewTiles"
            x:              root._thermalPipShown ? thermalOverlay.x + thermalOverlay.width + root._tilesMargin
                                                  : ScreenTools.defaultFontPixelWidth * 12
            y:              root.thermalPipTopInset + root._tilesMargin
            maximumWidth:   root.width - tiles.x - root.tilesRightInset - root._tilesMargin
            videoManager:   root.videoManager
            shown:          root.showTiles
        }
    }

    QGCLabel {
        id: fullScreenHint

        text: qsTr("Double-click to exit full screen")
        font.pointSize: ScreenTools.largeFontPointSize
        visible: root.interactive && root.videoManager.fullScreen
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

    // Over the tiles. It leaves a press to a tile by where the tiles are placed, not by whether one takes it: a press
    // on a tile that doesn't take it, as can happen right after a tap selects a tile, is nobody's.
    MouseArea {
        id: mouseArea

        property real _pressX: 0
        property real _pressY: 0
        property bool _dragging: false
        property bool _doubleClicked: false
        // A second finger made the press a pinch, which is neither a click nor a drag.
        property bool _pinched: false
        readonly property real _dragThreshold: 10

        function pinchBegan() {
            mouseArea._pinched = true
            if (mouseArea._dragging) {
                // The drag spans nothing: it ends where it began.
                root.dragEnded(mouseArea._pressX, mouseArea._pressY)
                mouseArea._dragging = false
            }
        }

        objectName:     "videoViewMouseArea"
        anchors.fill:   parent
        enabled:        root.interactive

        // Defer single-click handling so a double-click (fullscreen toggle) doesn't also
        // report an unintended click on its first click.
        Timer {
            id:         singleClickTimer
            objectName: "videoViewClickTimer"

            property real clickX: 0
            property real clickY: 0

            interval:   Qt.styleHints.mouseDoubleClickInterval
            repeat:     false

            onTriggered: root.clicked(singleClickTimer.clickX, singleClickTimer.clickY)
        }

        onDoubleClicked: (mouse) => {
            if (root.tileAt(mouse.x, mouse.y)) {
                mouse.accepted = false
                return
            }
            // Fires on the second press of a double-click. The second release still emits
            // onReleased, so flag it to prevent re-arming the single-click timer.
            mouseArea._doubleClicked = true
            singleClickTimer.stop()
            root.videoManager.fullScreen = !root.videoManager.fullScreen
        }

        onPressed: (mouse) => {
            // A press on a video tile is the tile's, which is below this area.
            if (root.tileAt(mouse.x, mouse.y)) {
                mouse.accepted = false
                return
            }
            mouseArea._pressX = mouse.x
            mouseArea._pressY = mouse.y
            mouseArea._dragging = false
            mouseArea._pinched = false
            // Clear any stale flag (e.g. double-click followed by drag releases through the
            // drag branch without consuming it). Safe: pressed is emitted before doubleClicked.
            mouseArea._doubleClicked = false
        }

        onPositionChanged: (mouse) => {
            if (mouseArea._pinched) {
                return
            }
            if (!mouseArea._dragging
                    && (Math.abs(mouse.x - mouseArea._pressX) >= mouseArea._dragThreshold
                        || Math.abs(mouse.y - mouseArea._pressY) >= mouseArea._dragThreshold)) {
                mouseArea._dragging = true
                root.dragStarted(mouseArea._pressX, mouseArea._pressY)
            }
            if (mouseArea._dragging) {
                root.dragMoved(mouse.x, mouse.y)
            }
        }

        onReleased: (mouse) => {
            if (mouseArea._pinched) {
                return
            }
            if (mouseArea._dragging) {
                root.dragEnded(mouse.x, mouse.y)
            } else if (mouseArea._doubleClicked) {
                // Second release of a double-click - fullscreen toggle already handled
                mouseArea._doubleClicked = false
            } else {
                singleClickTimer.clickX = mouse.x
                singleClickTimer.clickY = mouse.y
                singleClickTimer.restart()
            }
            mouseArea._dragging = false
        }
    }
}

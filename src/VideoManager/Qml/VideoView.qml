pragma ComponentBehavior: Bound

import QtQuick
import QtMultimedia

import QGroundControl
import QGroundControl.Controls

// The video of the primary stream or the UVC camera, fitted to the view, with grid lines, the telemetry overlay and the
// thermal stream. Where it is asked to, it shows the other video sources in tiles along its top.
Item {
    id: root

    property bool useSmallFont: true
    /// Height covered at the top of the view, which the thermal picture-in-picture and the tiles stay below.
    property real thermalPipTopInset: 0
    /// Whether the other video sources are shown in tiles: for a view with room for them, not a picture-in-picture.
    property bool showTiles: false
    /// Width covered at the right of the view, which the tiles stay clear of.
    property real tilesRightInset: 0
    /// The camera whose thermal mode and zoom the view follows: the active vehicle's current camera by default. Any
    /// object with the camera properties the view reads will do, as the tests' stand-ins.
    property var camera: root._activeVehicle && root._activeVehicle.cameraManager
                         ? root._activeVehicle.cameraManager.currentCameraInstance : null
    /// The manager whose video the view shows.
    property VideoManager videoManager: QGroundControl.videoManager

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
    readonly property VideoStreamController _primaryStream: root.videoManager.primaryStream
    // The primary stream is in a tile while the USB camera, selected in its own, is the main picture.
    readonly property bool _streamIsMainPicture: !root._primaryStream.tile
    // A lost stream's last frame stays up, under a notice, while it reconnects.
    readonly property bool _streamLost: root._streamIsMainPicture && root._primaryStream.reconnecting
    readonly property bool _showStreamLoader: root._streamIsMainPicture
                                              && (root.videoManager.decoding || root._streamLost)
    readonly property bool _showUvcLoader: root.videoManager.isUvc
    // A UVC camera's ratio is known once its first frame sets the output's implicit size; until then the video fills the
    // view.
    readonly property real _ar: root._showUvcLoader
                                ? ((cameraLoader.status === Loader.Ready && cameraLoader.item.implicitHeight > 0)
                                   ? cameraLoader.item.implicitWidth / cameraLoader.item.implicitHeight : 0)
                                : root.videoManager.aspectRatio
    readonly property int _fitMode: root._videoSettings.videoFit.rawValue
    readonly property bool _isModeFitWidth: root._fitMode === VideoSettings.VideoFitWidth
    readonly property bool _isModeFitHeight: root._fitMode === VideoSettings.VideoFitHeight
    readonly property bool _isModeFill: root._fitMode === VideoSettings.VideoFitFill
    readonly property bool _isModeNoCrop: root._fitMode === VideoSettings.VideoFitNoCrop
    // The outputs fill the view, and crop the video to it where the fitted video is larger.
    readonly property real _viewAr: root.width / root.height
    readonly property bool _cropsVideo: root._isModeFill
                                        || ((root._ar > 0)
                                            && ((root._isModeFitWidth && (root._viewAr > root._ar))
                                                || (root._isModeFitHeight && (root._viewAr < root._ar))))
    readonly property int _outputFillMode: root._cropsVideo ? VideoOutput.PreserveAspectCrop
                                                            : VideoOutput.PreserveAspectFit
    readonly property real _tilesMargin: ScreenTools.defaultFontPixelHeight * 0.5
    readonly property bool _thermalPipShown: thermalOverlay.visible && !!root.camera
                                             && root.camera.thermalMode === MavlinkCameraControlInterface.THERMAL_PIP

    /// Whether a tile is at @p x, @p y of the view: a press there is the tile's.
    function tileAt(x, y) {
        return tiles.visible && !!tiles.childAt(x - tiles.x, y - tiles.y)
    }

    // The grid lines span the fitted video, beyond the view where it crops.
    clip: true

    NoVideoPlaceholder {
        anchors.fill:   parent
        useSmallFont:   root.useSmallFont
        videoManager:   root.videoManager
        visible:        !root._showStreamLoader && !root._showUvcLoader
    }

    Rectangle {
        anchors.fill:   parent
        color:          "black"
        visible:        root._showStreamLoader || root._showUvcLoader

        Loader {
            anchors.fill:       parent
            visible:            root._showStreamLoader
            active:             root._streamIsMainPicture
            sourceComponent:    videoOutputComponent
        }

        Component {
            id: videoOutputComponent

            VideoStreamOutput {
                stream:     root._primaryStream
                fillMode:   root._outputFillMode
            }
        }

        //-- UVC Video (USB Camera or Video Device)
        Loader {
            id:                 cameraLoader
            anchors.fill:       parent
            visible:            root._showUvcLoader
            active:             root._showUvcLoader
            sourceComponent:    uvcVideoOutputComponent
        }

        Component {
            id: uvcVideoOutputComponent

            UVCVideoOutput {
                camera:     root.videoManager.uvcCamera
                fillMode:   root._outputFillMode
            }
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

        CameraPinchZoom {
            anchors.fill:   parent
            camera:         root.camera
        }
    }

    // Along the top, right of the thermal picture-in-picture, or in its place: clear of the Fly View's tool strip, as
    // that one is.
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

pragma ComponentBehavior: Bound

import QtQuick

import QGroundControl

// The video of the primary stream or the UVC camera, fitted to the view, with grid lines, the telemetry overlay and the
// thermal stream.
Item {
    id: root

    property bool useSmallFont: true
    /// Height covered at the top of the view, which the thermal picture-in-picture stays below.
    property real thermalPipTopInset: 0
    /// The camera whose thermal mode and zoom the view follows: the active vehicle's current camera by default. Any
    /// object with the camera properties the view reads will do, as the tests' stand-ins.
    property var camera: root._activeVehicle && root._activeVehicle.cameraManager
                         ? root._activeVehicle.cameraManager.currentCameraInstance : null

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

    readonly property VideoManager _videoManager: QGroundControl.videoManager
    readonly property VideoSettings _videoSettings: QGroundControl.settingsManager.videoSettings
    readonly property Vehicle _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    readonly property VideoStreamController _primaryStream: root._videoManager.primaryStream
    // A lost stream's last frame stays up, under a notice, while it reconnects.
    readonly property bool _streamLost: root._primaryStream.reconnecting
    readonly property bool _showStreamLoader: root._videoManager.decoding || root._streamLost
    readonly property bool _showUvcLoader: root._videoManager.isUvc
    // A UVC camera's ratio is known once its first frame sets the output's implicit size; until then the video fills the
    // view.
    readonly property real _ar: root._showUvcLoader
                                ? ((cameraLoader.status === Loader.Ready && cameraLoader.item.implicitHeight > 0)
                                   ? cameraLoader.item.implicitWidth / cameraLoader.item.implicitHeight : 0)
                                : root._videoManager.aspectRatio
    readonly property int _fitMode: root._videoSettings.videoFit.rawValue
    readonly property bool _isModeFitWidth: root._fitMode === VideoSettings.VideoFitWidth
    readonly property bool _isModeFitHeight: root._fitMode === VideoSettings.VideoFitHeight
    readonly property bool _isModeFill: root._fitMode === VideoSettings.VideoFitFill
    readonly property bool _isModeNoCrop: root._fitMode === VideoSettings.VideoFitNoCrop

    // The cropping fit modes size the video beyond the view.
    clip: true

    NoVideoPlaceholder {
        anchors.fill:   parent
        useSmallFont:   root.useSmallFont
        visible:        !root._showStreamLoader && !root._showUvcLoader
    }

    Rectangle {
        anchors.fill:   parent
        color:          "black"
        visible:        root._showStreamLoader || root._showUvcLoader

        Loader {
            anchors.fill:       videoContentArea
            visible:            root._showStreamLoader
            sourceComponent:    videoOutputComponent
        }

        Component {
            id: videoOutputComponent

            VideoStreamOutput {
                stream:                 root._primaryStream
                videoSink.subtitleText: root._videoManager.telemetryOverlayText
            }
        }

        //-- UVC Video (USB Camera or Video Device)
        Loader {
            id:                 cameraLoader
            anchors.fill:       videoContentArea
            visible:            root._showUvcLoader
            active:             root._showUvcLoader
            sourceComponent:    uvcVideoOutputComponent
        }

        Component {
            id: uvcVideoOutputComponent

            UVCVideoOutput {
                id: uvcOutput

                // Qt turns the subtitle with the output's orientation, so a rotated camera's would be sideways or
                // upside down.
                videoSink.subtitleText: uvcOutput.orientation === 0 ? root._videoManager.telemetryOverlayText : ""
            }
        }

        Item {
            id:                 videoContentArea
            width:              root.videoWidth
            height:             root.videoHeight
            anchors.centerIn:   parent

            VideoGridLines {
                anchors.fill:   parent
                visible:        root._videoSettings.gridLines.rawValue && !root._videoManager.fullScreen
            }
        }

        // Frozen video must not pass for live.
        VideoStatusLabel {
            objectName:         "videoLostLabel"
            anchors.centerIn:   parent
            useSmallFont:       root.useSmallFont
            text:               qsTr("VIDEO LOST - RECONNECTING")
            visible:            root._streamLost
        }

        ThermalVideoOverlay {
            objectName:     "videoViewThermal"
            camera:         root.camera
            pipTopInset:    root.thermalPipTopInset
        }

        CameraPinchZoom {
            anchors.fill:   parent
            camera:         root.camera
        }
    }
}

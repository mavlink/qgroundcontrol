pragma ComponentBehavior: Bound

import QtQuick

import QGroundControl
import QGroundControl.Controls

// The video of the primary stream or the UVC camera, fitted to the view, with grid lines and the thermal stream.
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
    readonly property bool _showStreamLoader: root._videoManager.decoding
    readonly property bool _showUvcLoader: root._videoManager.isUvc
    // A UVC camera's ratio is known once its first frame sets the output's implicit size; until then the video fills the
    // view.
    readonly property real _ar: root._showUvcLoader
                                ? ((cameraLoader.status === Loader.Ready && cameraLoader.item.implicitHeight > 0)
                                   ? cameraLoader.item.implicitWidth / cameraLoader.item.implicitHeight : 0)
                                : root._videoManager.aspectRatio
    readonly property bool _showGrid: root._videoSettings.gridLines.rawValue
    readonly property int _fitMode: root._videoSettings.videoFit.rawValue
    readonly property bool _isModeFitWidth: root._fitMode === VideoSettings.VideoFitWidth
    readonly property bool _isModeFitHeight: root._fitMode === VideoSettings.VideoFitHeight
    readonly property bool _isModeFill: root._fitMode === VideoSettings.VideoFitFill
    readonly property bool _isModeNoCrop: root._fitMode === VideoSettings.VideoFitNoCrop
    readonly property bool _hasZoom: !!root.camera && root.camera.hasZoom
    // Height of the thermal video blended over the main one, relative to the view.
    readonly property real _thermalBlendHeightFactor: 0.85
    // Pinch scale per camera zoom step, the same in and out.
    readonly property real _zoomStepScale: 1.25

    /// Sends the camera one zoom step (stepZoom() takes -1 or 1) toward the step count @p scale stands for, one per
    /// _zoomStepScale of pinch, given the @p sent steps of this pinch so far; returns the count sent after it. A step a
    /// pinch update skips goes with the next one.
    function _stepZoomToward(scale, sent) {
        const target = Math.round(Math.log(scale) / Math.log(root._zoomStepScale))
        if (!root._hasZoom || target === sent) {
            return sent
        }
        const direction = target > sent ? 1 : -1
        root.camera.stepZoom(direction)
        return sent + direction
    }

    // The cropping fit modes size the video beyond the view.
    clip: true

    Image {
        anchors.fill:   parent
        source:         "/res/NoVideoBackground.jpg"
        sourceSize:     Qt.size(root.width, root.height)
        fillMode:       Image.PreserveAspectCrop
        visible:        !root._showStreamLoader && !root._showUvcLoader

        Rectangle {
            anchors.centerIn:   parent
            width:              noVideoLabel.contentWidth + ScreenTools.defaultFontPixelHeight
            height:             noVideoLabel.contentHeight + ScreenTools.defaultFontPixelHeight
            radius:             ScreenTools.defaultFontPixelWidth / 2
            color:              "black"
            opacity:            0.5
        }

        QGCLabel {
            id:                 noVideoLabel
            text:               root._videoSettings.streamEnabled.rawValue ? qsTr("WAITING FOR VIDEO") : qsTr("VIDEO DISABLED")
            font.bold:          true
            color:              "white"
            font.pointSize:     root.useSmallFont ? ScreenTools.smallFontPointSize : ScreenTools.largeFontPointSize
            anchors.centerIn:   parent
        }
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
                stream: root._videoManager.primaryStream
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

            UVCVideoOutput { }
        }

        Item {
            id:                 videoContentArea
            width:              root.videoWidth
            height:             root.videoHeight
            anchors.centerIn:   parent
            visible:            root._showStreamLoader || root._showUvcLoader

            // grid lines
            Item {
                anchors.fill:   parent
                visible:        root._showGrid && !root._videoManager.fullScreen

                Rectangle {
                    color:  Qt.rgba(1, 1, 1, 0.5)
                    height: parent.height
                    width:  1
                    x:      parent.width * 0.33
                }
                Rectangle {
                    color:  Qt.rgba(1, 1, 1, 0.5)
                    height: parent.height
                    width:  1
                    x:      parent.width * 0.66
                }
                Rectangle {
                    color:  Qt.rgba(1, 1, 1, 0.5)
                    width:  parent.width
                    height: 1
                    y:      parent.height * 0.33
                }
                Rectangle {
                    color:  Qt.rgba(1, 1, 1, 0.5)
                    width:  parent.width
                    height: 1
                    y:      parent.height * 0.66
                }
            }
        }

        //-- Thermal Image
        Item {
            id:         thermalItem
            objectName: "videoViewThermal"
            width:      height * root._videoManager.thermalAspectRatio
            height: {
                if (!root.camera) {
                    return 0
                }
                switch (root.camera.thermalMode) {
                case MavlinkCameraControlInterface.THERMAL_FULL:
                    return parent.height
                case MavlinkCameraControlInterface.THERMAL_PIP:
                    return ScreenTools.defaultFontPixelHeight * 12
                default:
                    return parent.height * root._thermalBlendHeightFactor
                }
            }
            visible:    root._videoManager.hasThermal && !!root.camera
                        && root.camera.thermalMode !== MavlinkCameraControlInterface.THERMAL_OFF

            anchors {
                horizontalCenter:   parent.horizontalCenter
                verticalCenter:     parent.verticalCenter
                // Apply only in the pip state, which anchors the top left corner instead.
                topMargin:          root.thermalPipTopInset + (ScreenTools.defaultFontPixelHeight * 0.5)
                leftMargin:         ScreenTools.defaultFontPixelWidth * 12
            }

            states: [
                State {
                    name:   "pip"
                    when:   !!root.camera && root.camera.thermalMode === MavlinkCameraControlInterface.THERMAL_PIP

                    AnchorChanges {
                        target: thermalItem
                        anchors {
                            horizontalCenter:   undefined
                            verticalCenter:     undefined
                            top:                thermalItem.parent.top
                            left:               thermalItem.parent.left
                        }
                    }
                }
            ]

            Loader {
                anchors.fill:       parent
                opacity:            !root.camera ? 0
                                    : (root.camera.thermalMode === MavlinkCameraControlInterface.THERMAL_BLEND
                                       ? root.camera.thermalOpacity / 100 : 1.0)
                sourceComponent:    thermalOutputComponent

                Component {
                    id: thermalOutputComponent

                    VideoStreamOutput {
                        stream: root._videoManager.thermalStream
                    }
                }
            }
        }

        //-- Zoom
        PinchArea {
            id: pinchZoom

            // Zoom steps sent during the current pinch.
            property int zoom: 0

            anchors.fill:   parent
            enabled:        root._hasZoom

            onPinchStarted: pinchZoom.zoom = 0
            onPinchUpdated: (pinch) => { pinchZoom.zoom = root._stepZoomToward(pinch.scale, pinchZoom.zoom) }
        }
    }
}

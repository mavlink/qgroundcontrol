import QtQuick

import QGroundControl
import QGroundControl.Controls

// The video of the primary stream or the UVC camera, fitted to the view, with grid lines and the thermal stream.
Item {
    id:     root
    clip:   true

    property bool useSmallFont: true
    /// Height covered at the top of the view, which the thermal picture-in-picture stays below.
    property real thermalPipTopInset: 0
    readonly property var camera: _isCamera ? _dynamicCameras.cameras.get(_curCameraIndex) : null

    property double _ar:                (cameraLoader.visible && cameraLoader.status === Loader.Ready)
                                            ? cameraLoader.item.implicitWidth / cameraLoader.item.implicitHeight
                                            : QGroundControl.videoManager.aspectRatio
    property bool   _showGrid:          QGroundControl.settingsManager.videoSettings.gridLines.rawValue
    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property var    _dynamicCameras:    _activeVehicle ? _activeVehicle.cameraManager : null
    property bool   _connected:         _activeVehicle ? !_activeVehicle.communicationLost : false
    property int    _curCameraIndex:    _dynamicCameras ? _dynamicCameras.currentCamera : 0
    property bool   _isCamera:          _dynamicCameras ? _dynamicCameras.cameras.count > 0 : false
    property bool   _hasZoom:           camera && camera.hasZoom
    property int    _fitMode:           QGroundControl.settingsManager.videoSettings.videoFit.rawValue
    property bool   _showStreamLoader:  QGroundControl.videoManager.decoding
    property bool   _showUvcLoader:     QGroundControl.videoManager.isUvc

    property bool   _isMode_FIT_WIDTH:  _fitMode === 0
    property bool   _isMode_FIT_HEIGHT: _fitMode === 1
    property bool   _isMode_FILL:       _fitMode === 2
    property bool   _isMode_NO_CROP:    _fitMode === 3

    /// Size of the video fitted to the view: larger than the view where the fit mode crops, smaller where it doesn't.
    readonly property real videoWidth: {
        if(_ar != 0.0){
            if(_isMode_FIT_HEIGHT
                    || (_isMode_FILL && (root.width/root.height < _ar))
                    || (_isMode_NO_CROP && (root.width/root.height > _ar))){
                // This return value has different implications depending on the mode
                // For FIT_HEIGHT and FILL
                //    makes so the video width will be larger than (or equal to) the screen width
                // For NO_CROP Mode
                //    makes so the video width will be smaller than (or equal to) the screen width
                return root.height * _ar
            }
        }
        return root.width
    }
    readonly property real videoHeight: {
        if(_ar != 0.0){
            if(_isMode_FIT_WIDTH
                    || (_isMode_FILL && (root.width/root.height > _ar))
                    || (_isMode_NO_CROP && (root.width/root.height < _ar))){
                // This return value has different implications depending on the mode
                // For FIT_WIDTH and FILL
                //    makes so the video height will be larger than (or equal to) the screen height
                // For NO_CROP Mode
                //    makes so the video height will be smaller than (or equal to) the screen height
                return root.width * (1 / _ar)
            }
        }
        return root.height
    }

    property double _thermalHeightFactor: 0.85 //-- TODO

        Image {
            id:             noVideo
            anchors.fill:   parent
            source:         "/res/NoVideoBackground.jpg"
            fillMode:       Image.PreserveAspectCrop
            visible:        !_showStreamLoader && !_showUvcLoader

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
                text:               QGroundControl.settingsManager.videoSettings.streamEnabled.rawValue ? qsTr("WAITING FOR VIDEO") : qsTr("VIDEO DISABLED")
                font.bold:          true
                color:              "white"
                font.pointSize:     useSmallFont ? ScreenTools.smallFontPointSize : ScreenTools.largeFontPointSize
                anchors.centerIn:   parent
            }
        }

    Rectangle {
        id:             videoBackground
        anchors.fill:   parent
        color:          "black"
        visible:        _showStreamLoader || _showUvcLoader
        Loader {
            id:                 videoStreamLoader
            anchors.fill:       videoContentArea
            visible:            _showStreamLoader
            sourceComponent:    videoOutputComponent
        }
        Component {
            id: videoOutputComponent
            VideoStreamOutput {
                stream: QGroundControl.videoManager.primaryStream
            }
        }
        //-- UVC Video (USB Camera or Video Device)
        Loader {
            id:                 cameraLoader
            anchors.fill:       videoContentArea
            visible:            _showUvcLoader
            active:             _showUvcLoader
            sourceComponent:    uvcVideoOutputComponent
        }
        Component {
            id: uvcVideoOutputComponent
            UVCVideoOutput { }
        }

        Item {
            id:                 videoContentArea
            height:             root.videoHeight
            width:              root.videoWidth
            anchors.centerIn:   parent
            visible:           _showStreamLoader || _showUvcLoader

            // grid lines
            Item {
                anchors.fill:   parent
                visible:        _showGrid && !QGroundControl.videoManager.fullScreen

                Rectangle {
                    color:  Qt.rgba(1,1,1,0.5)
                    height: parent.height
                    width:  1
                    x:      parent.width * 0.33
                }
                Rectangle {
                    color:  Qt.rgba(1,1,1,0.5)
                    height: parent.height
                    width:  1
                    x:      parent.width * 0.66
                }
                Rectangle {
                    color:  Qt.rgba(1,1,1,0.5)
                    width:  parent.width
                    height: 1
                    y:      parent.height * 0.33
                }
                Rectangle {
                    color:  Qt.rgba(1,1,1,0.5)
                    width:  parent.width
                    height: 1
                    y:      parent.height * 0.66
                }
            }
        }

        //-- Thermal Image
        Item {
            id:                 thermalItem
            width:              height * QGroundControl.videoManager.thermalAspectRatio
            height:             camera ? (camera.thermalMode === MavlinkCameraControlInterface.THERMAL_FULL ? parent.height : (camera.thermalMode === MavlinkCameraControlInterface.THERMAL_PIP ? ScreenTools.defaultFontPixelHeight * 12 : parent.height * _thermalHeightFactor)) : 0
            anchors.centerIn:   parent
            visible:            QGroundControl.videoManager.hasThermal && camera && camera.thermalMode !== MavlinkCameraControlInterface.THERMAL_OFF
            function pipOrNot() {
                if(camera) {
                    if(camera.thermalMode === MavlinkCameraControlInterface.THERMAL_PIP) {
                        anchors.centerIn    = undefined
                        anchors.top         = parent.top
                        anchors.topMargin   = Qt.binding(() => root.thermalPipTopInset + (ScreenTools.defaultFontPixelHeight * 0.5))
                        anchors.left        = parent.left
                        anchors.leftMargin  = ScreenTools.defaultFontPixelWidth * 12
                    } else {
                        anchors.top         = undefined
                        anchors.topMargin   = undefined
                        anchors.left        = undefined
                        anchors.leftMargin  = undefined
                        anchors.centerIn    = parent
                    }
                }
            }
            Connections {
                target:                 camera
                function onThermalModeChanged() { thermalItem.pipOrNot() }
            }
            onVisibleChanged: {
                thermalItem.pipOrNot()
            }
            Loader {
                id:             thermalVideo
                anchors.fill:   parent
                opacity:        camera ? (camera.thermalMode === MavlinkCameraControlInterface.THERMAL_BLEND ? camera.thermalOpacity / 100 : 1.0) : 0
                sourceComponent: thermalOutputComponent

                Component {
                    id: thermalOutputComponent
                    VideoStreamOutput {
                        stream: QGroundControl.videoManager.thermalStream
                    }
                }
            }
        }
        //-- Zoom
        PinchArea {
            id:             pinchZoom
            enabled:        _hasZoom
            anchors.fill:   parent
            onPinchStarted: pinchZoom.zoom = 0
            onPinchUpdated: {
                if(_hasZoom) {
                    var z = 0
                    if(pinch.scale < 1) {
                        z = Math.round(pinch.scale * -10)
                    } else {
                        z = Math.round(pinch.scale)
                    }
                    if(pinchZoom.zoom != z) {
                        camera.stepZoom(z)
                    }
                }
            }
            property int zoom: 0
        }
    }
}

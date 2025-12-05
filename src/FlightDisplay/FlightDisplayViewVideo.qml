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

import QGroundControl
import QGroundControl.FlightDisplay
import QGroundControl.FlightMap

import QGroundControl.Controls




Item {
    id:     root
    clip:   true

    property bool useSmallFont: true

    property double _ar:                QGroundControl.videoManager.gstreamerEnabled
                                            ? QGroundControl.videoManager.videoSize.width / QGroundControl.videoManager.videoSize.height
                                            : QGroundControl.videoManager.aspectRatio
    property bool   _showGrid:          QGroundControl.settingsManager.videoSettings.gridLines.rawValue
    property var    _dynamicCameras:    globals.activeVehicle ? globals.activeVehicle.cameraManager : null
    property bool   _connected:         globals.activeVehicle ? !globals.activeVehicle.communicationLost : false
    property int    _curCameraIndex:    _dynamicCameras ? _dynamicCameras.currentCamera : 0
    property bool   _isCamera:          _dynamicCameras ? _dynamicCameras.cameras.count > 0 : false
    property var    _camera:            _isCamera ? _dynamicCameras.cameras.get(_curCameraIndex) : null
    property bool   _hasZoom:           _camera && _camera.hasZoom
    property int    _fitMode:           QGroundControl.settingsManager.videoSettings.videoFit.rawValue

    property bool   _isMode_FIT_WIDTH:  _fitMode === 0
    property bool   _isMode_FIT_HEIGHT: _fitMode === 1
    property bool   _isMode_FILL:       _fitMode === 2
    property bool   _isMode_NO_CROP:    _fitMode === 3

    function getWidth() {
        return videoBackground.getWidth()
    }
    function getHeight() {
        return videoBackground.getHeight()
    }

    property double _thermalHeightFactor: 0.85 //-- TODO

        Image {
            id:             noVideo
            anchors.fill:   parent
            source:         "/res/NoVideoBackground.jpg"
            fillMode:       Image.PreserveAspectCrop
            visible:        !(QGroundControl.videoManager.decoding)

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
        visible:        QGroundControl.videoManager.decoding
        function getWidth() {
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
        function getHeight() {
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
        Component {
            id: videoBackgroundComponent
            QGCVideoBackground {
                id:             videoContent
                objectName:     "videoContent"

                Connections {
                    target: QGroundControl.videoManager
                    function onImageFileChanged(filename) {
                        videoContent.grabToImage(function(result) {
                            if (!result.saveToFile(filename)) {
                                console.error('Error capturing video frame');
                            }
                        });
                    }
                }

                Rectangle {
                    color:  Qt.rgba(1,1,1,0.5)
                    height: parent.height
                    width:  1
                    x:      parent.width * 0.33
                    visible: _showGrid && !QGroundControl.videoManager.fullScreen
                }
                Rectangle {
                    color:  Qt.rgba(1,1,1,0.5)
                    height: parent.height
                    width:  1
                    x:      parent.width * 0.66
                    visible: _showGrid && !QGroundControl.videoManager.fullScreen
                }
                Rectangle {
                    color:  Qt.rgba(1,1,1,0.5)
                    width:  parent.width
                    height: 1
                    y:      parent.height * 0.33
                    visible: _showGrid && !QGroundControl.videoManager.fullScreen
                }
                Rectangle {
                    color:  Qt.rgba(1,1,1,0.5)
                    width:  parent.width
                    height: 1
                    y:      parent.height * 0.66
                    visible: _showGrid && !QGroundControl.videoManager.fullScreen
                }
            }
        }
        Loader {
            // GStreamer is causing crashes on Lenovo laptop OpenGL Intel drivers. In order to workaround this
            // we don't load a QGCVideoBackground object when video is disabled. This prevents any video rendering
            // code from running. Hence the Loader to completely remove it.
            height:             parent.getHeight()
            width:              parent.getWidth()
            anchors.centerIn:   parent
            visible:            QGroundControl.videoManager.decoding
            sourceComponent:    videoBackgroundComponent

            property bool videoDisabled: QGroundControl.settingsManager.videoSettings.videoSource.rawValue === QGroundControl.settingsManager.videoSettings.disabledVideoSource
        }

        //-- Backup Image
        Item {
            id:                 backupItem
            width:              height * QGroundControl.videoManager.aspectRatio
            height:             parent.height / 4
            visible:            QGroundControl.settingsManager.videoSettings.rtspUrlBackup.rawValue !== "" && QGroundControl.videoManager.decoding
            anchors.centerIn    : undefined
            anchors.verticalCenter: parent.verticalCenter
            anchors.left        : parent.left

            function undefinePosition() {
                anchors.centerIn    = undefined
                anchors.horizontalCenter = undefined
                anchors.top = undefined
                anchors.bottom = undefined
                anchors.verticalCenter = undefined
                anchors.left        = undefined
                anchors.right       = undefined
            }

            function leftPosition() {
                undefinePosition()

                anchors.verticalCenter = parent.verticalCenter
                anchors.left        = parent.left
            }

            function rightPosition() {
                undefinePosition()
                anchors.verticalCenter = parent.verticalCenter
                anchors.right = parent.right
            }

            function topPosition() {
                undefinePosition()
                anchors.horizontalCenter = parent.horizontalCenter
                anchors.top = parent.top
            }

            function bottomPosition() {
                undefinePosition()
                anchors.horizontalCenter = parent.horizontalCenter
                anchors.bottom = parent.bottom
            }

            QGCVideoBackground {
                id:             backupVideo
                objectName:     "backupVideo"
                anchors.fill:   parent
                opacity:        1.0
            }

            ToolTip {
                delay: 1000
                timeout: 3000
                visible: parent.visible
                anchors.centerIn: parent.centerIn
                text: qsTr("Right-click to open control menu")
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.RightButton
                onClicked: backupItemPositionMenu.popup()
            }

            QGCMenu {
                id: backupItemPositionMenu

                property bool hasAuxStream: QGroundControl.videoManager.hasAuxStream
                property bool usesPrimaryStream: QGroundControl.videoManager.isPrimaryStream

                QGCMenuItem {
                    text:           qsTr("Left")
                    onTriggered:    backupItem.leftPosition()
                }

                QGCMenuItem {
                    text:           qsTr("Right")
                    onTriggered:    backupItem.rightPosition()
                }

                QGCMenuItem {
                    text:           qsTr("Top")
                    onTriggered:    backupItem.topPosition()
                }

                QGCMenuItem {
                    text:           qsTr("Bottom")
                    onTriggered:    backupItem.bottomPosition()
                }

                QGCMenuSeparator {
                    visible:        true    // should be visible when backup stream is configured
                }

                QGCMenuItem {
                    id:             switchStreamMenuItem
                    text:           qsTr("Switch video streams")
                    visible:        true
                    enabled:        false
                    onTriggered:    QGroundControl.videoManager.isPrimaryStream = !QGroundControl.videoManager.isPrimaryStream
                }

                onHasAuxStreamChanged: {
                    switchStreamMenuItem.enabled = hasAuxStream
                }

                onUsesPrimaryStreamChanged: {
                    if (usesPrimaryStream) {
                        switchStreamMenuItem.text = qsTr("Switch to auxiliary video stream")
                    } else {
                        switchStreamMenuItem.text = qsTr("Switch to primary video stream")
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
                        _camera.stepZoom(z)
                    }
                }
            }
            property int zoom: 0
        }
    }
}

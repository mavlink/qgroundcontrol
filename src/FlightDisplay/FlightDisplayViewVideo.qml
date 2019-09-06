/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                          2.11
import QtQuick.Controls                 2.4

import QGroundControl                   1.0
import QGroundControl.FlightDisplay     1.0
import QGroundControl.FlightMap         1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.Controls          1.0
import QGroundControl.Palette           1.0
import QGroundControl.Vehicle           1.0
import QGroundControl.Controllers       1.0

Item {
    id:     root
    clip:   true
    property double _ar:                QGroundControl.videoManager.aspectRatio
    property bool   _showGrid:          QGroundControl.settingsManager.videoSettings.gridLines.rawValue > 0
    property var    _videoReceiver:     QGroundControl.videoManager.videoReceiver
    property var    _dynamicCameras:    activeVehicle ? activeVehicle.dynamicCameras : null
    property bool   _connected:         activeVehicle ? !activeVehicle.connectionLost : false
    property int    _curCameraIndex:    _dynamicCameras ? _dynamicCameras.currentCamera : 0
    property bool   _isCamera:          _dynamicCameras ? _dynamicCameras.cameras.count > 0 : false
    property var    _camera:            _isCamera ? _dynamicCameras.cameras.get(_curCameraIndex) : null
    property bool   _hasZoom:           _camera && _camera.hasZoom
    property int    _fitMode:           QGroundControl.settingsManager.videoSettings.videoFit.rawValue
    property bool   _thermaIsMain:      false
    property double _thermalHeightFactor: 0.85 //-- TODO

    Rectangle {
        id:             noVideo
        anchors.fill:   parent
        color:          Qt.rgba(0,0,0,0.75)
        visible:        !(_videoReceiver && _videoReceiver.videoRunning)
        QGCLabel {
            text:               QGroundControl.settingsManager.videoSettings.streamEnabled.rawValue ? qsTr("WAITING FOR VIDEO") : qsTr("VIDEO DISABLED")
            font.family:        ScreenTools.demiboldFontFamily
            color:              "white"
            font.pointSize:     mainIsMap ? ScreenTools.smallFontPointSize : ScreenTools.largeFontPointSize
            anchors.centerIn:   parent
        }
        MouseArea {
            anchors.fill: parent
            onDoubleClicked: {
                QGroundControl.videoManager.fullScreen = !QGroundControl.videoManager.fullScreen
            }
        }
    }
    Rectangle {
        id: _mainRectItem
        anchors.fill:   parent
        color:          "black"
        visible:        _videoReceiver && _videoReceiver.videoRunning
        function getWidth() {
            //-- Fit Width or Stretch
            if(_fitMode === 0 || _fitMode === 2) {
                return parent.width
            }
            //-- Fit Height
            return _ar != 0.0 ? parent.height * _ar : parent.width
        }
        function getHeight() {
            //-- Fit Height or Stretch
            if(_fitMode === 1 || _fitMode === 2) {
                return parent.height
            }
            //-- Fit Width
            return _ar != 0.0 ? parent.width * (1 / _ar) : parent.height
        }

        onWidthChanged: layoutPIPItems()
        onHeightChanged: layoutPIPItems()

        //-- Main Video
        Item {
            id:             videoItem
            height:         parent.height
            width:          parent.width
            x: 0
            y: 0
        QGCVideoBackground {
            id:                 videoContent
            anchors.centerIn:   parent
            width:              parent.width
            height:             parent.height
            receiver:           _videoReceiver
            display:            _videoReceiver && _videoReceiver.videoSurface
            visible:            _videoReceiver && _videoReceiver.videoRunning && !(QGroundControl.videoManager.hasThermal && _camera.thermalMode === QGCCameraControl.THERMAL_FULL)
            Connections {
                target:         _videoReceiver
                onImageFileChanged: {
                    videoContent.grabToImage(function(result) {
                        if (!result.saveToFile(_videoReceiver.imageFile)) {
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

        function setPIPForItems(mainView, previewItem) {
            if(mainView && _mainRectItem) {
                mainView.z = _mainRectItem.z + 1
                mainView.height = _mainRectItem.getHeight()
                mainView.width = _mainRectItem.getWidth()
                mainView.x = 0
                mainView.y = 0
            }
            if(previewItem && _mainRectItem) {
                if(mainView)
                    previewItem.z = mainView.z + 1

                previewItem.height = _camera ? (_camera.thermalMode === QGCCameraControl.THERMAL_FULL ? _mainRectItem.getHeight() : (_camera.thermalMode === QGCCameraControl.THERMAL_PIP ? ScreenTools.defaultFontPixelHeight * 12 : _mainRectItem.getHeight() * _thermalHeightFactor)) : 0
                previewItem.width = previewItem.height * QGroundControl.videoManager.thermalAspectRatio

                if(_camera && _camera.thermalMode === QGCCameraControl.THERMAL_PIP) {
                    previewItem.anchors.centerIn    = undefined
                    previewItem.anchors.top         = _mainRectItem.top
                    previewItem.anchors.topMargin   = mainWindow.header.height + (ScreenTools.defaultFontPixelHeight * 0.5)
                    previewItem.anchors.left        = _mainRectItem.left
                    previewItem.anchors.leftMargin  = ScreenTools.defaultFontPixelWidth * 12
                } else {
                    previewItem.anchors.top         = undefined
                    previewItem.anchors.topMargin   = undefined
                    previewItem.anchors.left        = undefined
                    previewItem.anchors.leftMargin  = undefined
                    previewItem.anchors.centerIn    = _mainRectItem
                }
            }
        }

        function layoutPIPItems() {
            var doThermalAsMain = _thermaIsMain && QGroundControl.videoManager.hasThermal && _camera.thermalMode === QGCCameraControl.THERMAL_PIP
            setPIPForItems(doThermalAsMain ? thermalItem : videoItem, doThermalAsMain ? videoItem : thermalItem);
        }

        //-- Thermal Image
        Item {
            id:                 thermalItem
            anchors.centerIn:   parent
            visible:            QGroundControl.videoManager.hasThermal && _camera.thermalMode !== QGCCameraControl.THERMAL_OFF

            Connections {
                target:                 _camera
                onThermalModeChanged:   _mainRectItem.layoutPIPItems()
            }
            onVisibleChanged: {
                _mainRectItem.layoutPIPItems()
            }
            QGCVideoBackground {
                id:                 thermalVideo
                anchors.centerIn:   parent
                width:              parent.width
                height:             parent.height
                receiver:           QGroundControl.videoManager.thermalVideoReceiver
                display:            QGroundControl.videoManager.thermalVideoReceiver ? QGroundControl.videoManager.thermalVideoReceiver.videoSurface : null
                opacity:            _camera ? (_camera.thermalMode === QGCCameraControl.THERMAL_BLEND ? _camera.thermalOpacity / 100 : 1.0) : 0
            }
            //-- swtich
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    _thermaIsMain = !_thermaIsMain
                    _mainRectItem.layoutPIPItems()
                }
            }
        }
        //-- Full screen toggle
        MouseArea {
            anchors.fill: parent
            onDoubleClicked: {
                QGroundControl.videoManager.fullScreen = !QGroundControl.videoManager.fullScreen
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

/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                          2.11
import QtQuick.Controls                 1.2

import QGroundControl                   1.0
import QGroundControl.FlightDisplay     1.0
import QGroundControl.FlightMap         1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.Controls          1.0
import QGroundControl.Palette           1.0
import QGroundControl.Vehicle           1.0
import QGroundControl.Controllers       1.0
import QGroundControl.SettingsManager   1.0

Item {
    id: root
    property var    _activeVehicle:         QGroundControl.multiVehicleManager.activeVehicle
    property bool   _showGrid:              QGroundControl.settingsManager.videoSettings.gridLines.rawValue > 0
    property var    _dynamicCameras:        _activeVehicle ? _activeVehicle.dynamicCameras : null
    property bool   _connected:             _activeVehicle ? !_activeVehicle.connectionLost : false
    property bool   _isCamera:              _dynamicCameras ? _dynamicCameras.cameras.count > 0 : false
    property var    _camera:                _isCamera ? _dynamicCameras.cameras.get(0) : null // Single camera support for the time being
    property real   _minLabel:              ScreenTools.defaultFontPixelWidth * 8
    property var    _arFact:                _camera && _camera.aspectRatio
    property double _ar:                    _arFact ? _arFact.rawValue : QGroundControl.settingsManager.videoSettings.aspectRatio.rawValue

    Rectangle {
        id:             noVideo
        anchors.fill:   parent
        color:          Qt.rgba(0,0,0,0.75)
        visible:        !QGroundControl.videoManager.videoReceiver.videoRunning
        QGCLabel {
            text:               qsTr("WAITING FOR VIDEO")
            font.family:        ScreenTools.demiboldFontFamily
            color:              "white"
            visible:            !_activeVehicle
            font.pointSize:     _mainIsMap ? ScreenTools.smallFontPointSize : ScreenTools.largeFontPointSize
            anchors.centerIn:   parent
        }
        QGCColoredImage {
            id:                 busyIndicator
            height:             parent.height * 0.25
            width:              height
            source:             "/qmlimages/MapSync.svg"
            sourceSize.height:  height
            fillMode:           Image.PreserveAspectFit
            mipmap:             true
            smooth:             true
            color:              qgcPal.colorBlue
            visible:            _activeVehicle
            anchors.centerIn:   parent
            RotationAnimation on rotation {
                loops:          Animation.Infinite
                from:           360
                to:             0
                duration:       740
                running:        noVideo.visible
            }
        }
        MouseArea {
            anchors.fill: parent
            onDoubleClicked: {
                QGroundControl.videoManager.fullScreen = !QGroundControl.videoManager.fullScreen
            }
        }
        PinchArea {
            anchors.fill: parent
            onPinchUpdated: {
                console.log('Pinch: ' + pinch.scale)
            }
        }
    }
    Rectangle {
        id:             videoRect
        anchors.fill:   parent
        color:          "black"
        visible:        QGroundControl.videoManager.videoReceiver.videoRunning
        QGCVideoBackground {
            id:             videoContent
            height:         parent.height
            width:          _ar != 0.0 ? height * _ar : parent.width
            anchors.centerIn: parent
            receiver:       QGroundControl.videoManager.videoReceiver
            display:        QGroundControl.videoManager.videoReceiver.videoSurface
            visible:        QGroundControl.videoManager.videoReceiver.videoRunning
            onWidthChanged: {
                if(_camera) {
                    _camera.videoSize = Qt.size(width, height);
                }
            }
            onHeightChanged: {
                if(_camera) {
                    _camera.videoSize = Qt.size(width, height);
                }
            }
            Connections {
                target:         QGroundControl.videoManager.videoReceiver
                onImageFileChanged: {
                    videoContent.grabToImage(function(result) {
                        if (!result.saveToFile(QGroundControl.videoManager.videoReceiver.imageFile)) {
                            console.error('Error capturing video frame');
                        }
                    });
                }
            }
        }
        Rectangle {
            color:  Qt.rgba(1,1,1,0.5)
            height: videoRect.height
            width:  1
            x:      videoRect.width * 0.33
            visible: _showGrid && !QGroundControl.videoManager.fullScreen
        }
        Rectangle {
            color:  Qt.rgba(1,1,1,0.5)
            height: videoRect.height
            width:  1
            x:      videoRect.width * 0.66
            visible: _showGrid && !QGroundControl.videoManager.fullScreen
        }
        Rectangle {
            color:  Qt.rgba(1,1,1,0.5)
            width:  videoRect.width
            height: 1
            y:      videoRect.height * 0.33
            visible: _showGrid && !QGroundControl.videoManager.fullScreen
        }
        Rectangle {
            color:  Qt.rgba(1,1,1,0.5)
            width:  videoRect.width
            height: 1
            y:      videoRect.height * 0.66
            visible: _showGrid && !QGroundControl.videoManager.fullScreen
        }
        MouseArea {
            anchors.fill:   videoRect
            onDoubleClicked: {
                QGroundControl.videoManager.fullScreen = !QGroundControl.videoManager.fullScreen
            }
        }
    }
}

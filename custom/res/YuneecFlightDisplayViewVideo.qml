/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                          2.3
import QtQuick.Controls                 1.2

import QGroundControl                   1.0
import QGroundControl.FlightDisplay     1.0
import QGroundControl.FlightMap         1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.Controls          1.0
import QGroundControl.Palette           1.0
import QGroundControl.Vehicle           1.0
import QGroundControl.Controllers       1.0

Item {
    id: root
    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property bool   _showGrid:          QGroundControl.settingsManager.videoSettings.gridLines.rawValue > 0
    property var    _dynamicCameras:    _activeVehicle ? _activeVehicle.dynamicCameras : null
    property bool   _connected:         _activeVehicle ? !_activeVehicle.connectionLost : false
    property bool   _isCamera:          _dynamicCameras ? _dynamicCameras.cameras.count > 0 : false
    property var    _camera:            _isCamera ? _dynamicCameras.cameras.get(0) : null // Single camera support for the time being
    property var    _meteringModeFact:  _camera && _camera.meteringMode
    property var    _expModeFact:       _camera && _camera.exposureMode
    property var    _arFact:            _camera && _camera.aspectRatio
    property bool   _cameraAutoMode:    _expModeFact  ? _expModeFact.rawValue === 0 : true
    property double _ar:                _arFact ? _arFact.rawValue : QGroundControl.settingsManager.videoSettings.aspectRatio.rawValue

    property real   spotSize:           48
    property bool   isSpot:             _camera && _cameraAutoMode && _meteringModeFact && _meteringModeFact.rawValue === 2
    property bool   isCenter:           _camera && _cameraAutoMode && _meteringModeFact && _meteringModeFact.rawValue === 1

    Rectangle {
        id:             noVideo
        anchors.fill:   parent
        color:          Qt.rgba(0,0,0,0.75)
        visible:        !QGroundControl.videoManager.videoReceiver.videoRunning
        QGCLabel {
            text:               qsTr("WAITING FOR VIDEO")
            font.family:        ScreenTools.demiboldFontFamily
            color:              "white"
            font.pointSize:     _mainIsMap ? ScreenTools.smallFontPointSize : ScreenTools.largeFontPointSize
            anchors.centerIn:   parent
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
            visible: _showGrid
        }
        Rectangle {
            color:  Qt.rgba(1,1,1,0.5)
            height: videoRect.height
            width:  1
            x:      videoRect.width * 0.66
            visible: _showGrid
        }
        Rectangle {
            color:  Qt.rgba(1,1,1,0.5)
            width:  videoRect.width
            height: 1
            y:      videoRect.height * 0.33
            visible: _showGrid
        }
        Rectangle {
            color:  Qt.rgba(1,1,1,0.5)
            width:  videoRect.width
            height: 1
            y:      videoRect.height * 0.66
            visible: _showGrid
        }
        //-- Spot Metering
        MouseArea {
            anchors.fill:   videoRect
            enabled:        isSpot
            onClicked: {
                _camera.spotArea = Qt.point(mouse.x, mouse.y)
                spotMetering.x = mouse.x - (spotMetering.width  / 2)
                spotMetering.y = mouse.y - (spotMetering.height / 2)
            }
        }
        Image {
            id:                 spotMetering
            x:                  _camera ? _camera.spotArea.x - (width  / 2) : 0
            y:                  _camera ? _camera.spotArea.y - (height / 2) : 0
            visible:            isSpot
            height:             videoContent.height / 16
            width:              height * 1.5
            antialiasing:       true
            mipmap:             true
            smooth:             true
            source:             "/typhoonh/img/spotArea.svg"
            fillMode:           Image.PreserveAspectFit
            sourceSize.width:   width
        }
        Image {
            id:                 centerMetering
            anchors.centerIn:   videoRect
            visible:            isCenter
            height:             spotSize * 1.5
            width:              height * 1.5
            antialiasing:       true
            mipmap:             true
            smooth:             true
            source:             "/typhoonh/img/centerArea.svg"
            fillMode:           Image.PreserveAspectFit
            sourceSize.height:  height
        }
    }
    //-- Camera Controller
    Loader {
        source:                 _dynamicCameras ? _dynamicCameras.controllerSource : ""
        visible:                !_mainIsMap && _dynamicCameras && _dynamicCameras.cameras.count && _connected
        anchors.right:          parent.right
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
        anchors.bottom:         parent.bottom
        anchors.bottomMargin:   ScreenTools.defaultFontPixelHeight * 2
    }
}

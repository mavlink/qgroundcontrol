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

import TyphoonHQuickInterface           1.0

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
    property real   _minLabel:          ScreenTools.defaultFontPixelWidth * 8

    property real   spotSize:           48
    property bool   isSpot:             _camera && _cameraAutoMode && _meteringModeFact && _meteringModeFact.rawValue === 2
    property bool   isCenter:           _camera && _cameraAutoMode && _meteringModeFact && _meteringModeFact.rawValue === 0
    property bool   isThermal:          !_mainIsMap && _camera && _camera.irValid && _camera.paramComplete && TyphoonHQuickInterface.thermalImagePresent && TyphoonHQuickInterface.videoReceiver

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
        MouseArea {
            anchors.fill: parent
            onDoubleClicked: {
                QGroundControl.videoManager.fullScreen = !QGroundControl.videoManager.fullScreen
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
            onClicked: {
                //-- Spot Metering
                if(isSpot) {
                    //-- Constrain to within video region
                    if(mouse.x >= videoContent.x && mouse.x < (videoContent.width + videoContent.x)) {
                        _camera.spotArea = Qt.point(mouse.x - videoContent.x, mouse.y)
                        spotMetering.x = mouse.x - (spotMetering.width  / 2)
                        spotMetering.y = mouse.y - (spotMetering.height / 2)
                    }
                }
            }
            onDoubleClicked: {
                QGroundControl.videoManager.fullScreen = !QGroundControl.videoManager.fullScreen
            }
        }
        Image {
            id:                 spotMetering
            x:                  _camera ? _camera.spotArea.x - (width  / 2) : 0
            y:                  _camera ? _camera.spotArea.y - (height / 2) : 0
            visible:            isSpot && !QGroundControl.videoManager.fullScreen
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
            visible:            isCenter && !QGroundControl.videoManager.fullScreen
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
    //-- Thermal Image
    Item {
        id:                 thermalItem
        width:              height * 1.333333
        height:             TyphoonHQuickInterface.thermalMode === TyphoonHQuickInterface.ThermalPIP ? ScreenTools.defaultFontPixelHeight * 20 : parent.height * 0.9444
        anchors.centerIn:   parent
        visible:            isThermal && TyphoonHQuickInterface.thermalMode !== TyphoonHQuickInterface.ThermalOff
        function pipOrNot() {
            if(TyphoonHQuickInterface.thermalMode === TyphoonHQuickInterface.ThermalPIP) {
                console.log('Pip Mode')
                anchors.centerIn    = undefined
                anchors.top         = parent.top
                anchors.topMargin   = ScreenTools.defaultFontPixelHeight * 5
                anchors.left        = parent.left
                anchors.leftMargin  = ScreenTools.defaultFontPixelWidth * 10
            } else {
                console.log('Non Pip Mode')
                anchors.top         = undefined
                anchors.topMargin   = undefined
                anchors.left        = undefined
                anchors.leftMargin  = undefined
                anchors.centerIn    = parent
            }
        }
        Connections {
            target:                 TyphoonHQuickInterface
            onThermalModeChanged:   thermalItem.pipOrNot()
        }
        QGCVideoBackground {
            id:             thermalVideo
            anchors.fill:   parent
            receiver:       TyphoonHQuickInterface.videoReceiver
            display:        TyphoonHQuickInterface.videoReceiver ? TyphoonHQuickInterface.videoReceiver.videoSurface : null
            opacity:        TyphoonHQuickInterface.thermalMode === TyphoonHQuickInterface.ThermalBlend ? 0.85 : 1.0
        }
    }
    //-- Area Temperature Indicator
    Image {
        visible:            isThermal && _camera.irROI && _camera.irROI.rawValue === 0
        height:             thermalItem.height * 0.25
        antialiasing:       true
        mipmap:             true
        smooth:             true
        source:             "/typhoonh/img/spot-area-ir.svg"
        fillMode:           Image.PreserveAspectFit
        sourceSize.height:  height
        anchors.centerIn:   thermalItem
        //-- Temperature
        Rectangle {
            id:                 aTempRect
            height:             ScreenTools.defaultFontPixelHeight * 2
            width:              Math.max(aTempLabel.width * 1.5, _minLabel)
            color:              Qt.rgba(0.5, 0, 0, 0.85)
            radius:             ScreenTools.defaultFontPixelWidth * 0.5
            anchors.centerIn:   parent
        }
        QGCLabel {
            id:                 aTempLabel
            text:               _camera ? _camera.irAverageTemp.toFixed(1) + '°C' : ""
            color:              "white"
            font.family:        ScreenTools.demiboldFontFamily
            anchors.centerIn:   aTempRect
        }
    }
    //-- Spot Temperature Indicator
    Image {
        id:                 spotTemp
        visible:            isThermal && _camera.irROI && _camera.irROI.rawValue !== 0
        height:             ScreenTools.defaultFontPixelHeight * 2
        antialiasing:       true
        mipmap:             true
        smooth:             true
        source:             "/typhoonh/img/spot-cross.svg"
        fillMode:           Image.PreserveAspectFit
        sourceSize.height:  height
        anchors.centerIn:   thermalItem
        //-- Temperature
        Rectangle {
            id:                 sTempRect
            height:             ScreenTools.defaultFontPixelHeight * 2
            width:              Math.max(sTempLabel.width * 1.5, _minLabel)
            color:              Qt.rgba(0.5, 0, 0, 0.85)
            radius:             ScreenTools.defaultFontPixelWidth * 0.5
            anchors.top:        parent.bottom
            anchors.topMargin:  ScreenTools.defaultFontPixelHeight
            anchors.horizontalCenter: parent.horizontalCenter
            QGCLabel {
                id:                 sTempLabel
                text:               _camera ? _camera.irCenterTemp.toFixed(1) + '°C' : ""
                color:              "white"
                font.family:        ScreenTools.demiboldFontFamily
                anchors.centerIn:   parent
            }
        }
    }
    //-- Color Bar
    Rectangle {
        id:                 colorBar
        anchors.left:       thermalItem.left
        anchors.leftMargin: ScreenTools.defaultFontPixelHeight * -4
        anchors.top:        parent.top
        anchors.topMargin:  ScreenTools.defaultFontPixelHeight * 6.5
        width:              ScreenTools.defaultFontPixelWidth  * 4
        height:             thermalItem.height * 0.5
        visible:            thermalItem.visible && _camera && _camera.irValid && TyphoonHQuickInterface.thermalMode !== TyphoonHQuickInterface.ThermalPIP
        color:              Qt.rgba(0,0,0,0)
        border.width:       1
        border.color:       qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0,0,0,0.35) : Qt.rgba(1,1,1,0.35)
        Image {
            anchors.fill:   parent
            anchors.margins: 1
            antialiasing:   true
            mipmap:         true
            smooth:         true
            source:         _camera ? _camera.palettetBar : ""
            fillMode:       Image.Stretch
            sourceSize.height:  height
        }
    }
    Rectangle {
        height:             ScreenTools.defaultFontPixelHeight * 2
        width:              Math.max(maxTempLabel.width * 1.5, _minLabel)
        visible:            colorBar.visible
        color:              Qt.rgba(0.5, 0, 0, 0.85)
        radius:             ScreenTools.defaultFontPixelWidth * 0.5
        anchors.top:        colorBar.top
        anchors.topMargin:  ScreenTools.defaultFontPixelHeight * -2.25
        anchors.horizontalCenter: colorBar.horizontalCenter
        QGCLabel {
            id:                 maxTempLabel
            text:               _camera ? _camera.irMaxTemp.toFixed(1) + '°C' : ""
            color:              "white"
            visible:            _camera && _camera.irValid
            font.family:        ScreenTools.demiboldFontFamily
            anchors.centerIn:   parent
        }
    }
    Rectangle {
        height:             ScreenTools.defaultFontPixelHeight * 2
        width:              Math.max(minTempLabel.width * 1.5, _minLabel)
        visible:            colorBar.visible
        color:              Qt.rgba(0, 0, 0.5, 0.85)
        radius:             ScreenTools.defaultFontPixelWidth * 0.5
        anchors.bottom:     colorBar.bottom
        anchors.bottomMargin: ScreenTools.defaultFontPixelHeight * -2.25
        anchors.horizontalCenter: colorBar.horizontalCenter
        QGCLabel {
            id:                 minTempLabel
            text:               _camera ? _camera.irMinTemp.toFixed(1) + '°C' : ""
            color:              "white"
            visible:            _camera && _camera.irValid
            font.family:        ScreenTools.demiboldFontFamily
            anchors.centerIn:   parent
        }
    }
}

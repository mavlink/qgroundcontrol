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
import QGroundControl.SettingsManager   1.0

import TyphoonHQuickInterface           1.0

Item {
    id: root
    property var    _activeVehicle:         QGroundControl.multiVehicleManager.activeVehicle
    property bool   _showGrid:              QGroundControl.settingsManager.videoSettings.gridLines.rawValue > 0
    property var    _dynamicCameras:        _activeVehicle ? _activeVehicle.dynamicCameras : null
    property bool   _connected:             _activeVehicle ? !_activeVehicle.connectionLost : false
    property bool   _isCamera:              _dynamicCameras ? _dynamicCameras.cameras.count > 0 : false
    property var    _camera:                _isCamera ? _dynamicCameras.cameras.get(0) : null // Single camera support for the time being
    property var    _meteringModeFact:      _camera && _camera.meteringMode
    property var    _expModeFact:           _camera && _camera.exposureMode
    property var    _arFact:                _camera && _camera.aspectRatio
    property bool   _cameraAutoMode:        _expModeFact  ? _expModeFact.rawValue === 0 : true
    property double _ar:                    _arFact ? _arFact.rawValue : QGroundControl.settingsManager.videoSettings.aspectRatio.rawValue
    property real   _minLabel:              ScreenTools.defaultFontPixelWidth * 8
    property double _thermalAspect:         _camera ? (_camera.isE10T ? 1.25 : 1.33) : 1
    property double _thermalHeightFactor:   _camera ? (_camera.isE10T ? 0.8333 : 0.9444) : 1
    property bool   _celcius:               QGroundControl.settingsManager.unitsSettings.temperatureUnits.rawValue === UnitsSettings.TemperatureUnitsCelsius

    property real   spotSize:               48
    property bool   isSpot:                 _camera && _cameraAutoMode && _meteringModeFact && _meteringModeFact.rawValue === 2
    property bool   isCenter:               _camera && _cameraAutoMode && _meteringModeFact && _meteringModeFact.rawValue === 0
    property bool   isThermal:              !_mainIsMap && _camera && _camera.irValid && _camera.paramComplete && TyphoonHQuickInterface.thermalImagePresent && TyphoonHQuickInterface.videoReceiver

    function getTemperature(tempC) {
        return (_celcius ? tempC : tempC * 1.8 + 32).toFixed(1) + (_celcius ? "°C" : "°F")
    }

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

            visible: {
                if(QGroundControl.videoManager.videoReceiver.videoRunning) {
                    if (isThermal && TyphoonHQuickInterface.thermalMode === TyphoonHQuickInterface.ThermalFull) {
                        return false;
                    }
                    return true;
                }
                return false;
            }

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
        width:              height * _thermalAspect
        height:             TyphoonHQuickInterface.thermalMode === TyphoonHQuickInterface.ThermalPIP ? ScreenTools.defaultFontPixelHeight * 16 : parent.height * _thermalHeightFactor
        anchors.centerIn:   parent
        visible:            isThermal && TyphoonHQuickInterface.thermalMode !== TyphoonHQuickInterface.ThermalOff
        function pipOrNot() {
            if(TyphoonHQuickInterface.thermalMode === TyphoonHQuickInterface.ThermalPIP) {
                anchors.centerIn    = undefined
                anchors.top         = parent.top
                anchors.topMargin   = ScreenTools.defaultFontPixelHeight * 7
                anchors.left        = parent.left
                anchors.leftMargin  = ScreenTools.defaultFontPixelWidth * 10
            } else {
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
        onVisibleChanged: {
            thermalItem.pipOrNot()
        }
        QGCVideoBackground {
            id:             thermalVideo
            anchors.fill:   parent
            receiver:       TyphoonHQuickInterface.videoReceiver
            display:        TyphoonHQuickInterface.videoReceiver ? TyphoonHQuickInterface.videoReceiver.videoSurface : null
            opacity:        TyphoonHQuickInterface.thermalMode === TyphoonHQuickInterface.ThermalBlend ? TyphoonHQuickInterface.thermalOpacity / 100 : 1.0
            Connections {
                target:         TyphoonHQuickInterface.videoReceiver
                onImageFileChanged: {
                    if(isThermal && TyphoonHQuickInterface.thermalMode !== TyphoonHQuickInterface.ThermalOff) {
                        thermalVideo.grabToImage(function(result) {
                            if (!result.saveToFile(TyphoonHQuickInterface.videoReceiver.imageFile)) {
                                console.error('Error capturing thermal video frame');
                            }
                        });
                    }
                }
            }
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
            text:               _camera ? getTemperature(_camera.irAverageTemp) : ""
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
                text:               _camera ? getTemperature(_camera.irCenterTemp) : ""
                color:              "white"
                font.family:        ScreenTools.demiboldFontFamily
                anchors.centerIn:   parent
            }
        }
    }
    //-- Color Bar
    Column {
        anchors.right:          thermalItem.left
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
        anchors.top:            _camera ? undefined : parent.top
        anchors.topMargin:      _camera ? undefined : ScreenTools.defaultFontPixelHeight * 6.5
        anchors.verticalCenter: _camera ? thermalItem.verticalCenter : undefined
        spacing:                ScreenTools.defaultFontPixelHeight * 0.25
        visible:                thermalItem.visible && _camera && _camera.irValid && TyphoonHQuickInterface.thermalMode !== TyphoonHQuickInterface.ThermalPIP
        Rectangle {
            height:             ScreenTools.defaultFontPixelHeight * 2
            width:              Math.max(maxTempLabel.width * 1.5, _minLabel)
            color:              Qt.rgba(0.5, 0, 0, 0.85)
            radius:             ScreenTools.defaultFontPixelWidth * 0.5
            anchors.horizontalCenter: parent.horizontalCenter
            QGCLabel {
                id:                 maxTempLabel
                text:               _camera ? getTemperature(_camera.irMaxTemp) : ""
                color:              "white"
                font.family:        ScreenTools.demiboldFontFamily
                anchors.centerIn:   parent
            }
        }
        Rectangle {
            width:              ScreenTools.defaultFontPixelWidth  * 4
            height:             thermalItem.height * 0.5
            color:              Qt.rgba(0,0,0,0)
            border.width:       1
            border.color:       qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0,0,0,0.35) : Qt.rgba(1,1,1,0.35)
            anchors.horizontalCenter: parent.horizontalCenter
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
            width:              Math.max(minTempLabel.width * 1.5, _minLabel)
            color:              Qt.rgba(0, 0, 0.5, 0.85)
            radius:             ScreenTools.defaultFontPixelWidth * 0.5
            anchors.horizontalCenter: parent.horizontalCenter
            QGCLabel {
                id:                 minTempLabel
                text:               _camera ? getTemperature(_camera.irMinTemp) : ""
                color:              "white"
                font.family:        ScreenTools.demiboldFontFamily
                anchors.centerIn:   parent
            }
        }
    }
}

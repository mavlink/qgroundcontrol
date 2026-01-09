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
    property bool   _hudCompact:        QGroundControl.settingsManager.videoSettings.hudCompact.rawValue
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
            Item {
                id:             videoContentContainer
                anchors.fill:   parent

                Loader {
                    id:             videoContentLoader
                    anchors.fill:   parent
                    active:         QGroundControl.videoManager.hasVideo
                    source:         "qrc:/qml/QGroundControl/FlightDisplay/QGCVideoBackground.qml"
                    onLoaded: {
                        if (item) {
                            item.streamObjectName = "videoContent"
                        }
                    }
                }

                Connections {
                    target: QGroundControl.videoManager
                    function onImageFileChanged(filename) {
                        if (!videoContentLoader.item) {
                            return
                        }
                        videoContentLoader.item.grabToImage(function(result) {
                            if (!result.saveToFile(filename)) {
                                console.error('Error capturing video frame');
                            }
                        });
                    }
                }

                // Holographic HUD overlay over the video, reusing the Video Grid Lines toggle
                // and styled similarly to the compass neon arcs.
                Rectangle {
                    color:  Qt.rgba(1,1,1,0.0)
                    height: parent.height
                    width:  1
                    x:      parent.width * 0.33
                    visible: false
                }
                Rectangle {
                    color:  Qt.rgba(1,1,1,0.0)
                    height: parent.height
                    width:  1
                    x:      parent.width * 0.66
                    visible: false
                }
                Rectangle {
                    color:  Qt.rgba(1,1,1,0.0)
                    width:  parent.width
                    height: 1
                    y:      parent.height * 0.33
                    visible: false
                }
                Rectangle {
                    color:  Qt.rgba(1,1,1,0.0)
                    width:  parent.width
                    height: 1
                    y:      parent.height * 0.66
                    visible: false
                }

                Item {
                    id:         holographicHud
                    anchors.fill: parent
                    visible:    _showGrid && !QGroundControl.videoManager.fullScreen
                    scale:      _hudCompact ? 0.65 : 1.0
                    transformOrigin: Item.Center

                    QGCPalette { id: hudPal; colorGroupEnabled: true }

                    // Central Focus SVG overlay
                    Image {
                        id:                 focusSvg
                        anchors.centerIn:   parent
                        width:              Math.min(parent.width, parent.height) * 0.55
                        height:             width
                        source:             "/qmlimages/Focus.svg"
                        fillMode:           Image.PreserveAspectFit
                        smooth:             true
                        opacity:            0.9
                    }

                    // Left roll tape - horizontal attitude (deg) from -15..+15
                    Item {
                        id:                     leftTape
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left:           parent.left
                        anchors.leftMargin:     parent.width * 0.08
                        width:                  parent.width * 0.12
                        height:                 parent.height * 0.60
                        visible:                globals.activeVehicle && globals.activeVehicle.altitudeAMSL

                        property var  _vehicle:    globals.activeVehicle
                        property real altSpan:     30
                        property real altValue:    (_vehicle && _vehicle.altitudeAMSL) ? _vehicle.altitudeAMSL.rawValue : NaN
                        property real altCenter:   isNaN(altValue) ? 0 : altValue
                        property real altMin:      altCenter - altSpan / 2
                        property real altMax:      altCenter + altSpan / 2
                        property real tapeMargin:  width * 0.02
                        property real tapeBoxW:    width * 0.55
                        property real cx:          tapeMargin + tapeBoxW

                        function valueToY(v) {
                            if (isNaN(v)) {
                                return height / 2
                            }
                            var clamped = Math.max(altMin, Math.min(altMax, v))
                            var ratio   = (clamped - altMin) / (altMax - altMin)
                            return height - (ratio * height)
                        }

                        onAltValueChanged: altScale.requestPaint()

                        Canvas {
                            id:             altScale
                            anchors.fill:   parent

                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.clearRect(0, 0, width, height)

                                if (isNaN(leftTape.altValue)) {
                                    return
                                }

                                ctx.strokeStyle = hudPal.brandingBlue
                                ctx.fillStyle   = hudPal.brandingBlue
                                ctx.lineWidth   = 1.3

                                var margin = width * 0.02
                                var boxW   = width * 0.55
                                var cx     = margin + boxW

                                // Main vertical line
                                ctx.beginPath()
                                ctx.moveTo(cx, 0)
                                ctx.lineTo(cx, height)
                                ctx.stroke()

                                // Ticks and labels
                                var step = 5
                                for (var v = leftTape.altMin; v <= leftTape.altMax; v += step) {
                                    var y = leftTape.valueToY(v)
                                    if (y < 0 || y > height) continue

                                    ctx.beginPath()
                                    ctx.moveTo(cx, y)
                                    ctx.lineTo(cx + width * 0.18, y)
                                    ctx.stroke()

                                    ctx.font         = (ScreenTools.defaultFontPixelHeight * 1.1) + "px sans-serif"
                                    ctx.textAlign    = "left"
                                    ctx.textBaseline = "middle"
                                    ctx.fillText(Math.round(v).toString(), cx + width * 0.22, y)
                                }
                            }
                        }

                        Rectangle {
                            id:                 altBox
                            width:              parent.width * 0.44
                            height:             ScreenTools.defaultFontPixelHeight * 2.2
                            radius:             ScreenTools.defaultFontPixelHeight * 0.25
                            color:              Qt.rgba(0.0, 0.8, 1.0, 0.16)
                            border.color:       hudPal.brandingBlue
                            border.width:       1.6
                            x:                  leftTape.cx + leftTape.tapeMargin
                            y:                  leftTape.valueToY(leftTape.altValue) - (height / 2)

                            QGCLabel {
                                anchors.centerIn:   parent
                                text: {
                                    if (isNaN(leftTape.altValue)) {
                                        return "-- m"
                                    }
                                    return leftTape.altValue.toFixed(1) + " m"
                                }
                                font.bold:          true
                                font.pointSize:     ScreenTools.defaultFontPointSize * 1.1
                                color:              hudPal.brandingBlue
                            }
                        }
                    }

                    // Right pitch tape - vertical attitude (deg) from -15..+15
                    Item {
                        id:                     rightTape
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right:          parent.right
                        anchors.rightMargin:    parent.width * 0.08
                        width:                  parent.width * 0.12
                        height:                 parent.height * 0.60
                        visible:                globals.activeVehicle && globals.activeVehicle.groundSpeed

                        property var  _vehicle:    globals.activeVehicle
                        property real speedSpan:   30
                        property real speedValue:  (_vehicle && _vehicle.groundSpeed) ? _vehicle.groundSpeed.rawValue : NaN
                        property real speedCenter: isNaN(speedValue) ? 0 : speedValue
                        property real speedMin:    speedCenter - speedSpan / 2
                        property real speedMax:    speedCenter + speedSpan / 2
                        property real tapeMargin:  width * 0.02
                        property real tapeBoxW:    width * 0.55
                        property real cx:          width - tapeMargin - tapeBoxW

                        function valueToY(v) {
                            if (isNaN(v)) {
                                return height / 2
                            }
                            var clamped = Math.max(speedMin, Math.min(speedMax, v))
                            var ratio   = (clamped - speedMin) / (speedMax - speedMin)
                            return height - (ratio * height)
                        }

                        onSpeedValueChanged: speedScale.requestPaint()

                        Canvas {
                            id:             speedScale
                            anchors.fill:   parent

                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.clearRect(0, 0, width, height)

                                if (isNaN(rightTape.speedValue)) {
                                    return
                                }

                                ctx.strokeStyle = hudPal.brandingBlue
                                ctx.fillStyle   = hudPal.brandingBlue
                                ctx.lineWidth   = 1.3

                                var margin = width * 0.02
                                var boxW   = width * 0.55
                                var cx     = width - margin - boxW

                                // Main vertical line
                                ctx.beginPath()
                                ctx.moveTo(cx, 0)
                                ctx.lineTo(cx, height)
                                ctx.stroke()

                                // Ticks and labels
                                var step = 5
                                for (var v = rightTape.speedMin; v <= rightTape.speedMax; v += step) {
                                    var y = rightTape.valueToY(v)
                                    if (y < 0 || y > height) continue

                                    ctx.beginPath()
                                    ctx.moveTo(cx, y)
                                    ctx.lineTo(cx - width * 0.18, y)
                                    ctx.stroke()

                                    ctx.font         = (ScreenTools.defaultFontPixelHeight * 1.1) + "px sans-serif"
                                    ctx.textAlign    = "right"
                                    ctx.textBaseline = "middle"
                                    ctx.fillText(Math.round(v).toString(), cx - width * 0.22, y)
                                }
                            }
                        }

                        Rectangle {
                            id:                 speedBox
                            width:              parent.width * 0.44
                            height:             ScreenTools.defaultFontPixelHeight * 2.2
                            radius:             ScreenTools.defaultFontPixelHeight * 0.25
                            color:              Qt.rgba(0.0, 0.8, 1.0, 0.16)
                            border.color:       hudPal.brandingBlue
                            border.width:       1.6
                            x:                  rightTape.cx - speedBox.width - rightTape.tapeMargin
                            y:                  rightTape.valueToY(rightTape.speedValue) - (height / 2)

                            QGCLabel {
                                anchors.centerIn:   parent
                                text: {
                                    if (isNaN(rightTape.speedValue)) {
                                        return "-- m/s"
                                    }
                                    return rightTape.speedValue.toFixed(1) + " m/s"
                                }
                                font.bold:          true
                                font.pointSize:     ScreenTools.defaultFontPointSize * 1.1
                                color:              hudPal.brandingBlue
                            }
                        }
                    }
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

        //-- Thermal Image
        Item {
            id:                 thermalItem
            width:              height * QGroundControl.videoManager.thermalAspectRatio
            height:             _camera ? (_camera.thermalMode === MavlinkCameraControl.THERMAL_FULL ? parent.height : (_camera.thermalMode === MavlinkCameraControl.THERMAL_PIP ? ScreenTools.defaultFontPixelHeight * 12 : parent.height * _thermalHeightFactor)) : 0
            anchors.centerIn:   parent
            visible:            QGroundControl.videoManager.hasThermal && _camera.thermalMode !== MavlinkCameraControl.THERMAL_OFF
            function pipOrNot() {
                if(_camera) {
                    if(_camera.thermalMode === MavlinkCameraControl.THERMAL_PIP) {
                        anchors.centerIn    = undefined
                        anchors.top         = parent.top
                        anchors.topMargin   = mainWindow.header.height + (ScreenTools.defaultFontPixelHeight * 0.5)
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
                target:                 _camera
                function onThermalModeChanged() { thermalItem.pipOrNot() }
            }
            onVisibleChanged: {
                thermalItem.pipOrNot()
            }
            Loader {
                id:             thermalVideoLoader
                anchors.fill:   parent
                opacity:        _camera ? (_camera.thermalMode === MavlinkCameraControl.THERMAL_BLEND ? _camera.thermalOpacity / 100 : 1.0) : 0
                active:         QGroundControl.videoManager.hasVideo
                source:         "qrc:/qml/QGroundControl/FlightDisplay/QGCVideoBackground.qml"
                onLoaded: {
                    if (item) {
                        item.streamObjectName = "thermalVideo"
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

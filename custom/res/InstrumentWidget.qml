/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief Fly View Instrument Widget
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick          2.4
import QtPositioning    5.2

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.Palette       1.0

import TyphoonHQuickInterface           1.0
import TyphoonHQuickInterface.Widgets   1.0

Item {
    id:     instrumentWidget
    height: mainRect.height
    width:  getPreferredInstrumentWidth() * 0.75

    property real _spacers:     ScreenTools.defaultFontPixelHeight * 0.5
    property real _distance:    0.0

    //-- Position from System GPS
    PositionSource {
        id:             positionSource
        updateInterval: 1000
        active:         !TyphoonHQuickInterface.hardwareGPS && activeVehicle && activeVehicle.homePositionAvailable
        onPositionChanged: {
            var gcs = positionSource.position.coordinate;
            var veh = activeVehicle ? activeVehicle.coordinate : QtPositioning.coordinate(0,0);
            _distance = activeVehicle ? gcs.distanceTo(veh) : 0.0;
            console.log("Qt PositionSource: " + gcs + veh + _distance)
        }
    }

    //-- Position from Controller GPS (M4)
    Connections {
        target: TyphoonHQuickInterface
        onControllerLocationChanged: {
            if(activeVehicle) {
                if(TyphoonHQuickInterface.latitude == 0.0 && TyphoonHQuickInterface.longitude == 0.0) {
                    _distance = 0.0
                } else {
                    var gcs = QtPositioning.coordinate(TyphoonHQuickInterface.latitude, TyphoonHQuickInterface.longitude, TyphoonHQuickInterface.altitude)
                    var veh = activeVehicle.coordinate;
                    _distance = gcs.distanceTo(veh);
                    console.log("M4 PositionSource: " + gcs + veh + _distance)
                }
            }
        }
    }

    Rectangle {
        id:             mainRect
        width:          parent.width
        height:         instrumentColumn.height
        radius:         8
        color:          Qt.rgba(0.15,0.15,0.25,0.75)
        border.width:   1
        border.color:   "black"
        Column {
            id:         instrumentColumn
            width:      parent.width
            spacing:    ScreenTools.defaultFontPixelHeight * 0.25
            anchors.verticalCenter: parent.verticalCenter
            Item {
                height: _spacers
                width:  1
            }
            //-- Attitude Indicator
            AttitudeWidget {
                id:             attitudeIndicator
                size:           parent.width * 0.95
                vehicle:        activeVehicle
                visible:        true
                anchors.horizontalCenter: parent.horizontalCenter
                MouseArea {
                    anchors.fill:   parent
                    enabled:        attitudeIndicator.visible
                    onClicked: {
                        attitudeIndicator.visible = false
                        compass.visible = true
                    }
                }
            }
            QGCCompassWidget {
                id:             compass
                size:           parent.width * 0.95
                vehicle:        activeVehicle
                visible:        false
                anchors.horizontalCenter: parent.horizontalCenter
                MouseArea {
                    anchors.fill:   parent
                    enabled:        compass.visible
                    onClicked: {
                        compass.visible = false
                        attitudeIndicator.visible = true
                    }
                }
            }
            Row {
                spacing:        ScreenTools.defaultFontPixelHeight * 0.25
                anchors.horizontalCenter: parent.horizontalCenter
                Image {
                    height:             ScreenTools.defaultFontPixelHeight
                    width:              height
                    sourceSize.width:   width
                    source:             "qrc:/typhoonh/UpArrow.svg"
                    fillMode:           Image.PreserveAspectFit
                    smooth:         true
                    mipmap:         true
                    antialiasing:   true
                    anchors.verticalCenter: parent.verticalCenter
                }
                QGCLabel {
                    text:           activeVehicle ? (isNaN(activeVehicle.altitudeRelative.rawValue) ? 0 : activeVehicle.altitudeRelative.rawValue.toFixed(0)) : 0
                    width:          ScreenTools.defaultFontPixelWidth * 4
                    font.family:    ScreenTools.demiboldFontFamily
                    horizontalAlignment: Text.AlignHCenter
                    anchors.verticalCenter: parent.verticalCenter
                }
                Image {
                    height:             ScreenTools.defaultFontPixelHeight
                    width:              height
                    sourceSize.width:   width
                    source:             "qrc:/typhoonh/RightArrow.svg"
                    fillMode:           Image.PreserveAspectFit
                    smooth:         true
                    mipmap:         true
                    antialiasing:   true
                    anchors.verticalCenter: parent.verticalCenter
                }
                QGCLabel {
                    text:           isNaN(_distance) ? 0.0 : _distance.toFixed(0)
                    width:          ScreenTools.defaultFontPixelWidth * 4
                    font.family:    ScreenTools.demiboldFontFamily
                    horizontalAlignment: Text.AlignHCenter
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            QGCLabel {
                text:           activeVehicle ? activeVehicle.groundSpeed.rawValue.toFixed(1) : "--"
                font.family:    ScreenTools.demiboldFontFamily
                font.pointSize: ScreenTools.largeFontPointSize
                anchors.horizontalCenter: parent.horizontalCenter
            }
            //-----------------------------------------------------------------
            Rectangle {
                height:         1
                width:          parent.width * 0.9
                color:          Qt.rgba(1,1,1,0.5)
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Item {
                height: _spacers * 2
                width:  1
            }
            //-- Camera Mode
            Rectangle {
                width:          ScreenTools.defaultFontPixelWidth  * 12
                height:         ScreenTools.defaultFontPixelHeight * 2
                radius:         width * 0.5
                color:          "black"
                anchors.horizontalCenter: parent.horizontalCenter
                Rectangle {
                    height:             parent.height
                    width:              parent.width * 0.5
                    radius:             width * 0.5
                    color:              TyphoonHQuickInterface.cameraMode === TyphoonHQuickInterface.CAMERA_MODE_PHOTO ? toolBar.colorGreen : "black"
                    anchors.left:       parent.left
                    QGCColoredImage {
                        height:             parent.height * 0.75
                        width:              height
                        sourceSize.width:   width
                        source:             "qrc:/typhoonh/camera.svg"
                        fillMode:           Image.PreserveAspectFit
                        color:              TyphoonHQuickInterface.cameraMode === TyphoonHQuickInterface.CAMERA_MODE_PHOTO ? "black" : toolBar.colorGrey
                        anchors.centerIn:   parent
                    }
                }
                Rectangle {
                    height:             parent.height
                    width:              parent.width * 0.5
                    radius:             width * 0.5
                    color:              TyphoonHQuickInterface.cameraMode === TyphoonHQuickInterface.CAMERA_MODE_VIDEO ? toolBar.colorGreen : "black"
                    anchors.right:      parent.right
                    QGCColoredImage {
                        height:             parent.height * 0.75
                        width:              height
                        sourceSize.width:   width
                        source:             "qrc:/typhoonh/video.svg"
                        fillMode:           Image.PreserveAspectFit
                        color:              TyphoonHQuickInterface.cameraMode === TyphoonHQuickInterface.CAMERA_MODE_VIDEO ? "black" : toolBar.colorGrey
                        anchors.centerIn:   parent
                    }
                }
                MouseArea {
                    anchors.fill:   parent
                    enabled:        TyphoonHQuickInterface.videoStatus !== TyphoonHQuickInterface.VIDEO_CAPTURE_STATUS_UNDEFINED
                    onClicked: {
                        TyphoonHQuickInterface.toggleMode()
                    }
                }
            }
            Item {
                height: _spacers * 2
                width:  1
            }
            Rectangle {
                height:             ScreenTools.defaultFontPixelHeight * 4
                width:              height
                radius:             width * 0.5
                color:              Qt.rgba(0.0,0.0,0.0,0.0)
                border.width:       1
                border.color:       toolBar.colorWhite
                anchors.horizontalCenter: parent.horizontalCenter
                QGCColoredImage {
                    height:             parent.height * 0.5
                    width:              height
                    sourceSize.width:   width
                    source:             "qrc:/typhoonh/video.svg"
                    fillMode:           Image.PreserveAspectFit
                    color:              TyphoonHQuickInterface.cameraMode === TyphoonHQuickInterface.CAMERA_MODE_VIDEO ? toolBar.colorGreen : toolBar.colorGrey
                    visible:            TyphoonHQuickInterface.videoStatus !== TyphoonHQuickInterface.VIDEO_CAPTURE_STATUS_RUNNING
                    anchors.centerIn:   parent
                    MouseArea {
                        anchors.fill:   parent
                        enabled:        TyphoonHQuickInterface.videoStatus === TyphoonHQuickInterface.VIDEO_CAPTURE_STATUS_STOPPED
                        onClicked: {
                            TyphoonHQuickInterface.startVideo()
                        }
                    }
                }
                Rectangle {
                    height:             parent.height * 0.5
                    width:              height
                    color:              TyphoonHQuickInterface.cameraMode === TyphoonHQuickInterface.CAMERA_MODE_VIDEO ? toolBar.colorRed : toolBar.colorGrey
                    visible:            TyphoonHQuickInterface.videoStatus === TyphoonHQuickInterface.VIDEO_CAPTURE_STATUS_RUNNING
                    anchors.centerIn:   parent
                    MouseArea {
                        anchors.fill:   parent
                        enabled:        TyphoonHQuickInterface.videoStatus === TyphoonHQuickInterface.VIDEO_CAPTURE_STATUS_RUNNING
                        onClicked: {
                            TyphoonHQuickInterface.stopVideo()
                        }
                    }
                }
            }
            Item {
                height: _spacers
                width:  1
            }
            Rectangle {
                height:             ScreenTools.defaultFontPixelHeight * 4
                width:              height
                radius:             width * 0.5
                color:              Qt.rgba(0.0,0.0,0.0,0.0)
                border.width:       1
                border.color:       toolBar.colorWhite
                anchors.horizontalCenter: parent.horizontalCenter
                QGCColoredImage {
                    height:             parent.height * 0.5
                    width:              height
                    sourceSize.width:   width
                    source:             "qrc:/typhoonh/camera.svg"
                    fillMode:           Image.PreserveAspectFit
                    color:              TyphoonHQuickInterface.cameraMode !== TyphoonHQuickInterface.CAMERA_MODE_UNDEFINED ? toolBar.colorGreen : toolBar.colorGrey
                    anchors.centerIn:   parent
                }
                MouseArea {
                    anchors.fill:   parent
                    enabled:        TyphoonHQuickInterface.cameraMode !== TyphoonHQuickInterface.CAMERA_MODE_UNDEFINED
                    onClicked: {
                        TyphoonHQuickInterface.takePhoto()
                    }
                }
            }
            Item {
                height: _spacers * 2
                width:  1
            }
            Rectangle {
                height:             ScreenTools.defaultFontPixelHeight * 2.5
                width:              height
                radius:             width * 0.5
                color:              Qt.rgba(0.0,0.0,0.0,0.0)
                border.width:       1
                border.color:       toolBar.colorWhite
                anchors.horizontalCenter: parent.horizontalCenter
                QGCColoredImage {
                    height:             parent.height * 0.65
                    width:              height
                    sourceSize.width:   width
                    source:             "qrc:/typhoonh/CogWheel.svg"
                    fillMode:           Image.PreserveAspectFit
                    color:              TyphoonHQuickInterface.cameraMode !== TyphoonHQuickInterface.CAMERA_MODE_UNDEFINED ? toolBar.colorWhite : toolBar.colorGrey
                    anchors.centerIn:   parent
                }
                MouseArea {
                    anchors.fill:   parent
                    onClicked: {
                        rootLoader.sourceComponent = messageArea
                    }
                }
            }
            Item {
                height:         _spacers * 2
                width:          1
            }
        }
    }

    Component {
        id: messageArea
        Rectangle {
            id:     messageRect
            width:  mainWindow.width  * 0.65
            height: mainWindow.height * 0.65
            radius: ScreenTools.defaultFontPixelWidth
            anchors.centerIn: parent
            color:          Qt.rgba(0,0,0,0.75)
            QGCLabel {
                text:               "Camera Settings"
                font.family:        ScreenTools.demiboldFontFamily
                font.pointSize:     ScreenTools.mediumFontPointSize
                anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.5
                anchors.top:        parent.top
                anchors.left:       parent.left
            }
            //-- Dismiss Window
            Image {
                anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.5
                anchors.top:        parent.top
                anchors.right:      parent.right
                width:              ScreenTools.defaultFontPixelHeight * 1.5
                height:             width
                sourceSize.height:  width
                source:             "/res/XDelete.svg"
                fillMode:           Image.PreserveAspectFit
                mipmap:             true
                smooth:             true
                MouseArea {
                    anchors.fill:   parent
                    onClicked: {
                        rootLoader.sourceComponent = null
                    }
                }
            }
            Component.onCompleted: {
                rootLoader.width  = messageRect.width
                rootLoader.height = messageRect.height
            }
        }
    }

}

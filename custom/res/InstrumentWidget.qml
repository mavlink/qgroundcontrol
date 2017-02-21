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

import QtQuick 2.4

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.Palette       1.0

import TyphoonHQuickInterface       1.0

Item {
    height: mainRect.height
    width:  getPreferredInstrumentWidth() * 0.65

    property real _spacers:     ScreenTools.defaultFontPixelHeight * 0.5

    Rectangle {
        id:             mainRect
        width:          parent.width
        height:         instrumentColumn.height
        radius:         4
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
            QGCAttitudeWidget {
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
                QGCLabel {
                    text:           qsTr("H:")
                    font.pointSize: ScreenTools.smallFontPointSize
                    anchors.verticalCenter: parent.verticalCenter
                }
                QGCLabel {
                    text:           activeVehicle ? activeVehicle.altitudeAMSL.rawValue.toFixed(1) : "----"
                    width:          ScreenTools.defaultFontPixelWidth * 4
                    font.family:    ScreenTools.demiboldFontFamily
                    horizontalAlignment: Text.AlignRight
                    anchors.verticalCenter: parent.verticalCenter
                }
                QGCLabel {
                    text:           qsTr("D:")
                    font.pointSize: ScreenTools.smallFontPointSize
                    anchors.verticalCenter: parent.verticalCenter
                }
                QGCLabel {
                    text:           activeVehicle ? activeVehicle.altitudeAMSL.rawValue.toFixed(1) : "----"
                    width:          ScreenTools.defaultFontPixelWidth * 4
                    font.family:    ScreenTools.demiboldFontFamily
                    horizontalAlignment: Text.AlignRight
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            QGCLabel {
                text:           activeVehicle ? activeVehicle.groundSpeed.rawValue.toFixed(1) : "--"
                font.family:    ScreenTools.demiboldFontFamily
                font.pointSize: ScreenTools.largeFontPointSize
                anchors.horizontalCenter: parent.horizontalCenter
            }
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
                width:          modeRow.width * 1.5
                height:         ScreenTools.defaultFontPixelHeight * 1.5
                radius:         width * 0.25
                color:          "black"
                anchors.horizontalCenter: parent.horizontalCenter
                Row {
                    id:         modeRow
                    height:     parent.height * 0.75
                    spacing:    ScreenTools.defaultFontPixelWidth * 3
                    anchors.centerIn: parent
                    Rectangle {
                        height:             parent.height
                        width:              height
                        radius:             width * 0.5
                        color:              TyphoonHQuickInterface.cameraMode === TyphoonHQuickInterface.CAMERA_MODE_PHOTO ? toolBar.colorGreen : "black"
                        anchors.verticalCenter: parent.verticalCenter
                        QGCColoredImage {
                            height:             parent.height
                            width:              height
                            sourceSize.width:   width
                            source:             "qrc:/typhoonh/camera.svg"
                            fillMode:           Image.PreserveAspectFit
                            color:              TyphoonHQuickInterface.cameraMode === TyphoonHQuickInterface.CAMERA_MODE_PHOTO ? toolBar.colorWhite : toolBar.colorGrey
                            anchors.centerIn:   parent
                        }
                    }
                    Rectangle {
                        height:             parent.height
                        width:              height
                        radius:             width * 0.5
                        color:              TyphoonHQuickInterface.cameraMode === TyphoonHQuickInterface.CAMERA_MODE_VIDEO ? toolBar.colorGreen : "black"
                        anchors.verticalCenter: parent.verticalCenter
                        QGCColoredImage {
                            height:             parent.height
                            width:              height
                            sourceSize.width:   width
                            source:             "qrc:/typhoonh/video.svg"
                            fillMode:           Image.PreserveAspectFit
                            color:              TyphoonHQuickInterface.cameraMode === TyphoonHQuickInterface.CAMERA_MODE_VIDEO ? toolBar.colorWhite : toolBar.colorGrey
                            anchors.centerIn:   parent
                        }
                    }
                }
            }
            Item {
                height: _spacers * 2
                width:  1
            }
            Rectangle {
                height:             ScreenTools.defaultFontPixelHeight * 3
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
                    color:              TyphoonHQuickInterface.cameraMode === TyphoonHQuickInterface.CAMERA_MODE_VIDEO ? toolBar.colorGreen : toolBar.colorGrey
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
                height:             ScreenTools.defaultFontPixelHeight * 3
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
                height:             ScreenTools.defaultFontPixelHeight * 2
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
            }
            Item {
                height:         _spacers * 2
                width:          1
            }
        }
    }
}

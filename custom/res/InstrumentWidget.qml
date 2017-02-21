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
    width:  getPreferredInstrumentWidth()

    property real _spacing:     ScreenTools.defaultFontPixelHeight * 0.25

    Rectangle {
        id:             mainRect
        width:          parent.width
        height:         instrumentColumn.height * 1.25
        radius:         4
        color:          Qt.rgba(0.5,0.5,0.5,0.75)
        border.width:   1
        border.color:   "black"
        Column {
            id:         instrumentColumn
            width:      parent.width
            spacing:    _spacing
            anchors.verticalCenter: parent.verticalCenter
            Item {
                height: ScreenTools.defaultFontPixelHeight * 0.5
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
                    spacing:    ScreenTools.defaultFontPixelWidth
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
                height: ScreenTools.defaultFontPixelHeight * 0.5
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
                    anchors.centerIn:   parent
                }
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
                    color:              toolBar.colorGreen
                    anchors.centerIn:   parent
                }
            }
            Item {
                height: ScreenTools.defaultFontPixelHeight * 0.5
                width:  1
            }
        }
    }
}

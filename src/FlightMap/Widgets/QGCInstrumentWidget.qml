/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief QGC Compass Widget
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick 2.4

import QGroundControl.Controls 1.0
import QGroundControl.ScreenTools 1.0

Item {
    id:     root
    height: size

    property alias  heading:        compass.heading
    property alias  rollAngle:      attitude.rollAngle
    property alias  pitchAngle:     attitude.pitchAngle
    property real   altitude:       0
    property real   groundSpeed:    0
    property real   airSpeed:       0
    property real   climbRate:      0
    property real   size:           ScreenTools.defaultFontPixelSize * (10)
    property bool   isSatellite:    false
    property bool   active:         false

    property bool   _isVisible:     true

    //-- Instrument Pannel
    Rectangle {
        id:                     instrumentPannel
        height:                 instruments.height + ScreenTools.defaultFontPixelSize
        width:                  root.size
        radius:                 root.size / 2
        visible:                _isVisible
        color:                  isSatellite ? Qt.rgba(1,1,1,0.75) : Qt.rgba(0,0,0,0.75)
        anchors.right:          parent.right
        anchors.verticalCenter: parent.verticalCenter
        Column {
            id:                 instruments
            width:              parent.width
            spacing:            ScreenTools.defaultFontPixelSize * 0.33
            anchors.verticalCenter: parent.verticalCenter
            //-- Attitude Indicator
            QGCAttitudeWidget {
                id:             attitude
                size:           parent.width * 0.9
                active:         root.active
                anchors.horizontalCenter: parent.horizontalCenter
            }
            //-- Altitude
            Rectangle {
                height:         1
                width:          parent.width * 0.9
                color:          isSatellite ? Qt.rgba(0,0,0,0.25) : Qt.rgba(1,1,1,0.25)
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Row {
                width: root.size * 0.8
                anchors.horizontalCenter: parent.horizontalCenter
                QGCLabel {
                    text:           "H"
                    font.pixelSize: ScreenTools.defaultFontPixelSize * 1.25
                    width:          parent.width * 0.45
                    color:          isSatellite ? "black" : "white"
                    horizontalAlignment: TextEdit.AlignHCenter
                }
                QGCLabel {
                    text:           altitude.toFixed(1)
                    font.pixelSize: ScreenTools.defaultFontPixelSize * 1.25
                    font.weight:    Font.DemiBold
                    width:          parent.width * 0.549
                    color:          isSatellite ? "black" : "white"
                    horizontalAlignment: TextEdit.AlignHCenter
                }
            }
            //-- Ground Speed
            Rectangle {
                height:         1
                width:          parent.width * 0.9
                color:          isSatellite ? Qt.rgba(0,0,0,0.25) : Qt.rgba(1,1,1,0.25)
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Row {
                width: root.size * 0.8
                anchors.horizontalCenter: parent.horizontalCenter
                QGCLabel {
                    text:           "GS"
                    font.pixelSize: ScreenTools.defaultFontPixelSize * 0.75
                    width:          parent.width * 0.45
                    color:          isSatellite ? "black" : "white"
                    horizontalAlignment: TextEdit.AlignHCenter
                }
                QGCLabel {
                    text:           groundSpeed.toFixed(1)
                    font.pixelSize: ScreenTools.defaultFontPixelSize * 0.75
                    font.weight:    Font.DemiBold
                    width:          parent.width * 0.549
                    color:          isSatellite ? "black" : "white"
                    horizontalAlignment: TextEdit.AlignHCenter
                }
            }
            //-- Air Speed
            Rectangle {
                height:         1
                width:          parent.width * 0.9
                color:          isSatellite ? Qt.rgba(0,0,0,0.25) : Qt.rgba(1,1,1,0.25)
                anchors.horizontalCenter: parent.horizontalCenter
                visible:        airSpeed > 0
            }
            Row {
                width: root.size * 0.8
                anchors.horizontalCenter: parent.horizontalCenter
                visible:        airSpeed > 0
                QGCLabel {
                    text:           "AS"
                    font.pixelSize: ScreenTools.defaultFontPixelSize * 0.75
                    width:          parent.width * 0.45
                    color:          isSatellite ? "black" : "white"
                    horizontalAlignment: TextEdit.AlignHCenter
                }
                QGCLabel {
                    text:           airSpeed.toFixed(1)
                    font.pixelSize: ScreenTools.defaultFontPixelSize * 0.75
                    font.weight:    Font.DemiBold
                    width:          parent.width * 0.549
                    color:          isSatellite ? "black" : "white"
                    horizontalAlignment: TextEdit.AlignHCenter
                }
            }
            //-- Climb Rate
            Rectangle {
                height:         1
                width:          parent.width * 0.9
                color:          isSatellite ? Qt.rgba(0,0,0,0.25) : Qt.rgba(1,1,1,0.25)
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Row {
                width: root.size * 0.8
                anchors.horizontalCenter: parent.horizontalCenter
                QGCLabel {
                    text:           "VS"
                    font.pixelSize: ScreenTools.defaultFontPixelSize * 0.75
                    width:          parent.width * 0.45
                    color:          isSatellite ? "black" : "white"
                    horizontalAlignment: TextEdit.AlignHCenter
                }
                QGCLabel {
                    text:           climbRate.toFixed(1)
                    font.pixelSize: ScreenTools.defaultFontPixelSize * 0.75
                    font.weight:    Font.DemiBold
                    width:          parent.width * 0.549
                    color:          isSatellite ? "black" : "white"
                    horizontalAlignment: TextEdit.AlignHCenter
                }
            }
            //-- Compass
            Rectangle {
                height:         1
                width:          parent.width * 0.9
                color:          isSatellite ? Qt.rgba(0,0,0,0.25) : Qt.rgba(1,1,1,0.25)
                anchors.horizontalCenter: parent.horizontalCenter
            }
            QGCCompassWidget {
                id:             compass
                size:           parent.width * 0.9
                active:         root.active
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
        MouseArea {
            anchors.fill: parent
            onClicked: {
                _isVisible = !_isVisible
            }
        }
    }

    //-- Show Instruments
    Rectangle {
        id:                     openButton
        anchors.right:          parent.right
        anchors.verticalCenter: parent.verticalCenter
        height:                 ScreenTools.defaultFontPixelSize * 2
        width:                  ScreenTools.defaultFontPixelSize * 2
        radius:                 ScreenTools.defaultFontPixelSize / 3
        visible:                !_isVisible
        color:                  isSatellite ? Qt.rgba(1,1,1,0.5) : Qt.rgba(0,0,0,0.5)
        Image {
            width:              parent.width  * 0.75
            height:             parent.height * 0.75
            source:             "/qmlimages/buttonLeft.svg"
            mipmap:             true
            fillMode:           Image.PreserveAspectFit
            anchors.verticalCenter:     parent.verticalCenter
            anchors.horizontalCenter:   parent.horizontalCenter
        }
        MouseArea {
            anchors.fill: parent
            onClicked: {
                _isVisible = !_isVisible
            }
        }
    }

}

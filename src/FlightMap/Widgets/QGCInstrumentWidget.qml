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

    property real   size:           ScreenTools.defaultFontPixelSize * (10)
    property bool   isSatellite:    false
    property bool   active:         false

    property bool   _isVisible:     true

    //-- Instrument Pannel
    Rectangle {
        id:                 instrumentPannel
        anchors.right:      parent.right
        anchors.bottom:     parent.bottom
        height:             root.size
        width:              instruments.width + 8
        radius:             root.size / 2
        visible:            _isVisible
        color:              isSatellite ? Qt.rgba(1,1,1,0.5) : Qt.rgba(0,0,0,0.5)
        Row {
            id:                 instruments
            height:             parent.height
            spacing:            4
            anchors.horizontalCenter:   parent.horizontalCenter
            QGCAttitudeWidget {
                id:             attitude
                size:           parent.height * 0.9
                active:         root.active
                anchors.verticalCenter: parent.verticalCenter
            }
            QGCCompassWidget {
                id:             compass
                size:           parent.height * 0.9
                active:         root.active
                anchors.verticalCenter: parent.verticalCenter
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
        id:                 openButton
        anchors.right:      parent.right
        anchors.bottom:     parent.bottom
        height:             24
        width:              24
        radius:             4
        visible:            !_isVisible
        color:              isSatellite ? Qt.rgba(1,1,1,0.5) : Qt.rgba(0,0,0,0.5)
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

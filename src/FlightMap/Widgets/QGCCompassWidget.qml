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
import QtGraphicalEffects 1.0

import QGroundControl.Controls 1.0
import QGroundControl.ScreenTools 1.0

Item {
    id:                     root

    property bool active:   false  ///< true: actively connected to data provider, false: show inactive control
    property real heading:  0
    property real size:     _defaultSize

    property real _defaultSize: ScreenTools.defaultFontPixelSize * (10)
    property real _sizeRatio:   ScreenTools.isTinyScreen ? (size / _defaultSize) * 0.5 : size / _defaultSize
    property int  _fontSize:    ScreenTools.defaultFontPixelSize * _sizeRatio

    width:                  size
    height:                 size

    Rectangle {
        id:             borderRect
        anchors.fill:   parent
        radius:         width / 2
        color:          "black"
    }

    Item {
        id:             instrument
        anchors.fill:   parent
        visible:        false

        Image {
            id:                 pointer
            source:             "/qmlimages/compassInstrumentAirplane.svg"
            mipmap:             true
            width:              size * 0.75
            fillMode:           Image.PreserveAspectFit
            anchors.centerIn:   parent
            transform: Rotation {
                origin.x:       pointer.width  / 2
                origin.y:       pointer.height / 2
                angle:          heading
            }
        }

        Image {
            id:                 compassDial
            source:             "/qmlimages/compassInstrumentDial.svg"
            mipmap:             true
            fillMode:           Image.PreserveAspectFit
            anchors.fill:       parent
        }

        Rectangle {
            anchors.centerIn:   parent
            width:              size * 0.35
            height:             size * 0.2
            border.color:       Qt.rgba(1,1,1,0.15)
            color:              Qt.rgba(0,0,0,0.65)

            QGCLabel {
                text:           active ? heading.toFixed(0) : qsTr("OFF")
                font.weight:    active ? Font.DemiBold : Font.Light
                font.pixelSize: _fontSize < 1 ? 1 : _fontSize;
                color:          "white"
                anchors.centerIn: parent
            }
        }
    }

    Rectangle {
        id:             mask
        anchors.fill:   instrument
        radius:         width / 2
        color:          "black"
        visible:        false
    }

    OpacityMask {
        anchors.fill:   instrument
        source:         instrument
        maskSource:     mask
    }

}

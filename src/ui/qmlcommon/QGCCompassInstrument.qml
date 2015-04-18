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
 *   @brief QGC Compass
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick 2.4
import QGroundControl.Controls 1.0
import QGroundControl.ScreenTools 1.0

QGCMovableItem {
    property ScreenTools screenTools: ScreenTools { }
    id:                     root
    property real heading:  0
    property real size:     120
    property real _fontSize: screenTools.dpiAdjustedPointSize(size * 12 / 120)
    width:                  size
    height:                 size
    Rectangle {
        id:                 compassBack
        anchors.fill:       parent
        color:              "#212121"
    }
    Image {
        id:                 pointer
        source:             "/qml/compassInstrumentAirplane.svg"
        mipmap:             true
        width:              root.width * 0.75
        fillMode:           Image.PreserveAspectFit
        anchors.centerIn:   parent
        transform: Rotation {
            origin.x:   pointer.width  / 2
            origin.y:   pointer.height / 2
            angle:      -heading
        }
    }
    Image {
        id:                 compassDial
        source:             "/qml/compassInstrumentDial.svg"
        mipmap:             true
        width:              root.width
        fillMode:           Image.PreserveAspectFit
        anchors.centerIn:   parent
    }
    Rectangle {
        anchors.centerIn:   root
        width:              size * 0.35
        height:             size * 0.2
        border.color:       Qt.rgba(1,1,1,0.15)
        color:              Qt.rgba(0,0,0,0.65)
        QGCLabel {
            text:           heading.toFixed(0)
            font.weight:    Font.DemiBold
            font.pointSize: _fontSize < 1 ? 1 : _fontSize;
            color: "white"
            anchors.centerIn: parent
        }
    }
}



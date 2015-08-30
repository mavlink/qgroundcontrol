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
 *   @brief QGC Attitude Instrument
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick 2.4
import QGroundControl.Controls 1.0

QGCMovableItem {
    id: root
    property real rollAngle:    0
    property real pitchAngle:   0
    property bool showPitch:    true
    property real size

    width:  size
    height: size

    //----------------------------------------------------
    //-- Artificial Horizon
    QGCArtificialHorizon {
        rollAngle:      root.rollAngle
        pitchAngle:     root.pitchAngle
        anchors.fill:   parent
    }
    //----------------------------------------------------
    //-- Pointer
    Image {
        id:         pointer
        source:     "/qmlimages/attitudePointer.svg"
        mipmap:     true
        fillMode:   Image.PreserveAspectFit
        anchors.fill: parent
    }
    //----------------------------------------------------
    //-- Instrument Dial
    Image {
        id:         instrumentDial
        source:     "/qmlimages/attitudeDial.svg"
        mipmap:     true
        fillMode:   Image.PreserveAspectFit
        anchors.fill: parent
        transform: Rotation {
            origin.x: root.width  / 2
            origin.y: root.height / 2
            angle: -rollAngle
        }
    }
    //----------------------------------------------------
    //-- Pitch
    QGCPitchIndicator {
        id:                 pitchWidget
        visible:            root.showPitch
        size:               root.size * 0.65
        anchors.verticalCenter: parent.verticalCenter
        pitchAngle:         root.pitchAngle
        rollAngle:          root.rollAngle
        color:              Qt.rgba(0,0,0,0)
    }
    //----------------------------------------------------
    //-- Cross Hair
    Image {
        id:                 crossHair
        anchors.centerIn:   parent
        source:             "/qmlimages/crossHair.svg"
        mipmap:             true
        width:              size * 0.75
        fillMode:           Image.PreserveAspectFit
    }
    //----------------------------------------------------
    //-- Instrument Pannel
    Image {
        id:             pannel
        source:         "/qmlimages/attitudeInstrument.svg"
        mipmap:         true
        fillMode:       Image.PreserveAspectFit
        anchors.fill:   parent
    }
}

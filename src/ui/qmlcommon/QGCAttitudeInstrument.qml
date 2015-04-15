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

Item {
    id: root
    property real rollAngle :   0
    property real pitchAngle:   0
    property real size:         100

    width:  size
    height: size

    //----------------------------------------------------
    //-- Artificial Horizon
    QGCArtificialHorizon {
        rollAngle:          root.rollAngle
        pitchAngle:         root.pitchAngle
    }
    //----------------------------------------------------
    //-- Pointer
    Image {
        id:         pointer
        source:     "/qml/attitudePointer.svg"
        width:      root.width
        mipmap:     true
        fillMode:   Image.PreserveAspectFit
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
    }
    //----------------------------------------------------
    //-- Instrument Dial
    Image {
        id:         instrumentDial
        source:     "/qml/attitudeDial.svg"
        mipmap:     true
        width:      root.width
        fillMode:   Image.PreserveAspectFit
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        transform: Rotation {
            origin.x: root.width  / 2
            origin.y: root.height / 2
            angle: -rollAngle
        }
    }
    //----------------------------------------------------
    //-- Pitch
    QGCPitchWidget {
        id:                 pitchWidget
        size:               parent.width * 0.65
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
        source:             "/qml/crossHair.svg"
        mipmap:             true
        width:              parent.width * 0.75
        fillMode:           Image.PreserveAspectFit
    }
    //----------------------------------------------------
    //-- Instrument Pannel
    Image {
        id:         pannel
        width:      parent.width
        source:     "/qml/attitudeInstrument.svg"
        mipmap:     true
        fillMode:   Image.PreserveAspectFit
        anchors.centerIn: parent
    }
}

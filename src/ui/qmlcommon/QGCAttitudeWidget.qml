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
 *   @brief QGC Attitude Widget
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick 2.4

Item {
    id: root
    property real rollAngle :   0
    property real pitchAngle:   0
    property bool showAttitude: true

    anchors.fill: parent

    QGCArtificialHorizon {
        rollAngle:          root.rollAngle
        pitchAngle:         root.pitchAngle
    }

    Image {
        id: rollDial
        visible: root.showAttitude
        anchors { bottom: root.verticalCenter; horizontalCenter: parent.horizontalCenter}
        source: "/qml/rollDialWhite.svg"
        mipmap: true
        width:  260
        fillMode: Image.PreserveAspectFit
        transform: Rotation {
            origin.x: rollDial.width  / 2
            origin.y: rollDial.height
            angle:   -rollAngle
        }
    }

    Image {
        id: pointer
        visible: root.showAttitude
        anchors { bottom: root.verticalCenter; horizontalCenter: parent.horizontalCenter}
        source:             "/qml/rollPointerWhite.svg"
        mipmap:             true
        width:              rollDial.width
        fillMode:           Image.PreserveAspectFit
    }

    Image {
        id:                 crossHair
        visible:            root.showAttitude
        anchors.centerIn:   parent
        source:             "/qml/crossHair.svg"
        mipmap:             true
        width:              260
        fillMode:           Image.PreserveAspectFit
    }
}

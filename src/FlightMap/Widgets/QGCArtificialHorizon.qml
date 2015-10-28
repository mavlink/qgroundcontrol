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
 *   @brief QGC Artificial Horizon
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick 2.4

Item {
    id: root
    property real rollAngle :   0
    property real pitchAngle:   0
    clip:           true
    anchors.fill:   parent

    property real angularScale: pitchAngle * root.height / 45

    Item {
        id: artificialHorizon
        width:  root.width  * 4
        height: root.height * 4
        anchors.centerIn: parent
        Rectangle {
            id: sky
            anchors.fill: parent
            smooth: true
            antialiasing: true
            gradient: Gradient {
                GradientStop { position: 0.25; color: Qt.hsla(0.6, 1.0, 0.25) }
                GradientStop { position: 0.5;  color: Qt.hsla(0.6, 0.5, 0.55) }
            }
        }
        Rectangle {
            id: ground
            height: sky.height / 2
            anchors {
                left:   sky.left;
                right:  sky.right;
                bottom: sky.bottom
            }
            smooth: true
            antialiasing: true
            gradient: Gradient {
                GradientStop { position: 0.0;  color: Qt.hsla(0.25,  0.5, 0.45) }
                GradientStop { position: 0.25; color: Qt.hsla(0.25, 0.75, 0.25) }
            }
        }
        transform: [
            Translate {
                y:  angularScale
            },
            Rotation {
                origin.x: artificialHorizon.width  / 2
                origin.y: artificialHorizon.height / 2
                angle:    -rollAngle
            }]
    }
}

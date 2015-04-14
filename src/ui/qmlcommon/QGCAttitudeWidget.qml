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
 *   @brief QGC Main Flight Display
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick 2.4

Item {
    id: root
    property real rollAngle : 0
    property real pitchAngle: 0
    property real backgroundOpacity: 1
    property bool displayBackground: true
    property bool useWhite: true

    anchors.fill: parent

    Item {
        id: background
        width:  parent.width  * 4
        height: parent.height * 4
        anchors.centerIn: parent

        Rectangle {
            anchors.fill: parent
            color: Qt.rgba(0,0,0,0)
            visible: displayBackground

            Rectangle {
                id: sky
                visible: displayBackground
                anchors.fill: parent
                smooth: true
                antialiasing: true
                gradient: Gradient {
                    GradientStop { position: 0.25; color: Qt.hsla(0.6, 1.0, 0.25, backgroundOpacity) }
                    GradientStop { position: 0.5;  color: Qt.hsla(0.6, 0.5, 0.75, backgroundOpacity) }
                }
            }

            Rectangle {
                id: ground
                visible: displayBackground
                height: sky.height / 2
                anchors {
                    left:   sky.left;
                    right:  sky.right;
                    bottom: sky.bottom
                }
                smooth: true
                antialiasing: true
                gradient: Gradient {
                    GradientStop { position: 0.0;  color: Qt.hsla(0.25,  0.5, 0.45, backgroundOpacity) }
                    GradientStop { position: 0.25; color: Qt.hsla(0.25, 0.75, 0.25, backgroundOpacity) }
                }
            }

            transform: [
                Translate {
                    y:  (root.visible && root.displayBackground) ? pitchAngle * 4 : 0
                },
                Rotation {
                    origin.x: background.width  / 2
                    origin.y: background.height / 2
                    angle: (root.visible && root.displayBackground) ? -rollAngle : 0
                }]
        }

    }

    Image {
        id: rollDial
        anchors { bottom: root.verticalCenter; horizontalCenter: parent.horizontalCenter}
        source: useWhite ? "/qml/rollDialWhite.svg" : "/qml/rollDial.svg"
        mipmap: true
        width:  260
        fillMode: Image.PreserveAspectFit
        transform: Rotation {
            origin.x: rollDial.width  / 2
            origin.y: rollDial.height
            angle: -rollAngle
        }
    }

    Image {
        id: pointer
        anchors { bottom: root.verticalCenter; horizontalCenter: parent.horizontalCenter}
        source: useWhite ? "/qml/rollPointerWhite.svg" : "/qml/rollPointer.svg"
        smooth:   true
        width: rollDial.width
        fillMode: Image.PreserveAspectFit
    }

}

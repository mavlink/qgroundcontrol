/****************************************************************************
 *
 * (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 * @file
 *   @author Gus Grubba <gus@auterion.com>
 */

import QtQuick 2.11

Item {
    id: root
    property real   rollAngle :     0
    property real   pitchAngle:     0
    property color  skyColor1:      Qt.hsla(0.6, 1.0, 0.25)
    property color  skyColor2:      Qt.hsla(0.6, 0.5, 0.55)
    property color  groundColor1:   Qt.hsla(0.25,  0.5, 0.45)
    property color  groundColor2:   Qt.hsla(0.25, 0.75, 0.25)

    clip:           true
    anchors.fill:   parent

    property real angularScale: pitchAngle * root.height / 45

    Item {
        id:     artificialHorizon
        width:  root.width  * 4
        height: root.height * 8
        anchors.centerIn: parent
        Rectangle {
            id:             sky
            anchors.fill:   parent
            smooth:         true
            antialiasing:   true
            gradient: Gradient {
                GradientStop { position: 0.25; color: root.skyColor1 }
                GradientStop { position: 0.5;  color: root.skyColor2 }
            }
        }
        Rectangle {
            id:             ground
            height:         sky.height / 2
            anchors {
                left:       sky.left;
                right:      sky.right;
                bottom:     sky.bottom
            }
            smooth:         true
            antialiasing:   true
            gradient: Gradient {
                GradientStop { position: 0.0;  color: root.groundColor1 }
                GradientStop { position: 0.25; color: root.groundColor2 }
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

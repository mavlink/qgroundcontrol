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
 *   @brief QGC Pitch Indicator
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick 2.1

Rectangle {
    property real pitchAngle:      0
    property real rollAngle:       0
    property real _reticleHeight:  1
    property real _reticleSpacing: 20
    property real _reticleSlot:    _reticleSpacing + _reticleHeight
    height: 130
    width:  parent.width
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.verticalCenter:   parent.verticalCenter
    clip: true
    color: Qt.rgba(0,0,0,0)
    Item {
        height: parent.height
        width:  parent.width
        Column{
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            spacing: _reticleSpacing
            Repeater {
                model: 36
                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: ((modelData * 5 - 90) % 10) === 0 ? 50 : 30
                    height: _reticleHeight
                    color: "white"
                    antialiasing: true
                    smooth: true
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.horizontalCenterOffset: -40
                        anchors.verticalCenter: parent.verticalCenter
                        smooth: true
                        font.weight: Font.DemiBold
                        text: -(modelData * 5 - 90)
                        color: "white"
                        visible: ((modelData * 5 - 90) % 10) === 0
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.horizontalCenterOffset: 40
                        anchors.verticalCenter: parent.verticalCenter
                        smooth: true
                        font.weight: Font.DemiBold
                        text: -(modelData * 5 - 90)
                        color: "white"
                        visible: ((modelData * 5 - 90) % 10) === 0
                    }
                }
            }
        }
        transform: [ Translate {
                y: (pitchAngle * _reticleSlot / 5) - (_reticleSlot / 2)
                }]
    }
    transform: [
        Rotation {
            origin.x: width / 2
            origin.y: height / 2
            angle:    -rollAngle
            }
    ]
}

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
 *   @brief QGC Speed Indicator
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick 2.4

Rectangle {
    id: root
    property real speed:           0
    property real _reticleSpacing: 14
    property real _reticleHeight:  1
    property real _reticleSlot:    _reticleSpacing + _reticleHeight

    anchors.verticalCenter: parent.verticalCenter

    height: parent.height * 0.75 > 280 ? 280 : parent.height * 0.75
    clip:   true
    smooth: true
    radius: 5
    border.color: Qt.rgba(1,1,1,0.25)
    gradient: Gradient {
        GradientStop { position: 0.0; color: Qt.rgba(0,0,0,0.65) }
        GradientStop { position: 0.5; color: Qt.rgba(0,0,0,0.25) }
        GradientStop { position: 1.0; color: Qt.rgba(0,0,0,0.65) }
    }
    Column{
        width: parent.width
        anchors.verticalCenter: parent.verticalCenter
        spacing: _reticleSpacing
        Repeater {
            model: 40
            Rectangle {
                property int _speed: -(index - 20)
                width:  (_speed % 5 === 0) ? 15 : 30
                anchors.right: parent.right
                height: _reticleHeight
                color:  Qt.rgba(1,1,1,0.35)
                Text {
                    visible: (_speed % 5 === 0)
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.horizontalCenterOffset: -25
                    anchors.verticalCenter:   parent.verticalCenter
                    antialiasing: true
                    font.weight: _speed < 0 ? Font.Light : Font.DemiBold
                    text: _speed < 0 ? -_speed : _speed
                    color: _speed < 0 ? "#ef2526" : "white"
                    style: Text.Outline
                    styleColor: Qt.rgba(0,0,0,0.25)
                }
            }
        }
        transform: Translate {
            y: (speed * _reticleSlot) - (_reticleSlot / 2)
        }
    }
}

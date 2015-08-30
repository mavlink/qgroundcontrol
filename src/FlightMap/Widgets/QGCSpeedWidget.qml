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
import QGroundControl.ScreenTools 1.0
import QGroundControl.Controls 1.0

Rectangle {
    id: root
    property real speed:           0
    property real _reticleSpacing: ScreenTools.defaultFontPixelSize * (0.83)
    property real _reticleHeight:  ScreenTools.defaultFontPixelSize * (0.166)
    property real _reticleSlot:    _reticleSpacing + _reticleHeight
    anchors.verticalCenter: parent.verticalCenter
    z:10
    smooth: true
    radius: ScreenTools.defaultFontPixelSize * (0.42)
    border.color: Qt.rgba(1,1,1,0.25)
    gradient: Gradient {
        GradientStop { position: 0.0; color: Qt.rgba(0,0,0,0.65) }
        GradientStop { position: 0.5; color: Qt.rgba(0,0,0,0.25) }
        GradientStop { position: 1.0; color: Qt.rgba(0,0,0,0.65) }
    }
    Rectangle {
        id:     clipRect
        height: parent.height - ScreenTools.defaultFontPixelSize * (0.42)
        width:  parent.width
        clip:   true
        color:  Qt.rgba(0,0,0,0);
        anchors.verticalCenter: parent.verticalCenter
        Column{
            id: col
            width: parent.width
            anchors.verticalCenter: parent.verticalCenter
            spacing: _reticleSpacing
            Repeater {
                model: 40
                Rectangle {
                    property int _speed: -(index - 20)
                    width:  (_speed % 5 === 0) ? ScreenTools.defaultFontPixelSize * (0.83) : ScreenTools.defaultFontPixelSize * (1.25)
                    anchors.right: parent.right
                    height: _reticleHeight
                    color:  Qt.rgba(1,1,1,0.35)
                    QGCLabel {
                        visible: (_speed % 5 === 0)
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.horizontalCenterOffset: ScreenTools.defaultFontPixelSize * (-2.5)
                        anchors.verticalCenter:   parent.verticalCenter
                        antialiasing: true
                        font.weight: Font.DemiBold
                        text:  _speed
                        color: _speed < 0 ? "#f8983a" : "white"
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
}

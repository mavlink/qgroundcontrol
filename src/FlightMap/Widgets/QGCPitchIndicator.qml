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
import QGroundControl.ScreenTools 1.0
import QGroundControl.Controls 1.0

Rectangle {
    property real pitchAngle:       0
    property real rollAngle:        0
    property real size:             _defaultSize
    property real _reticleHeight:   1
    property real _reticleSpacing:  size * 0.15
    property real _reticleSlot:     _reticleSpacing + _reticleHeight
    property real _longDash:        size * 0.40
    property real _shortDash:       size * 0.25
    property real _fontSize:        ScreenTools.defaultFontPixelSize * (size / _defaultSize)

    property real _defaultSize:     ScreenTools.isAndroid ? 300 : 100

    height: size
    width:  size
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.verticalCenter:   parent.verticalCenter
    clip: true
    Item {
        height: parent.height
        width:  parent.width
        Column{
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter:   parent.verticalCenter
            spacing: _reticleSpacing
            Repeater {
                model: 36
                Rectangle {
                    property int _pitch: -(modelData * 5 - 90)
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: (_pitch % 10) === 0 ? _longDash : _shortDash
                    height: _reticleHeight
                    color: "white"
                    antialiasing: true
                    smooth: true
                    QGCLabel {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.horizontalCenterOffset: -(_longDash * 0.8)
                        anchors.verticalCenter: parent.verticalCenter
                        smooth: true
                        font.weight: Font.DemiBold
                        font.pixelSize: _fontSize < 1 ? 1 : _fontSize;
                        text: _pitch
                        color: "white"
                        visible: (_pitch != 0) && ((_pitch % 10) === 0)
                    }
                    QGCLabel {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.horizontalCenterOffset: (_longDash * 0.8)
                        anchors.verticalCenter: parent.verticalCenter
                        smooth: true
                        font.weight: Font.DemiBold
                        font.pixelSize: _fontSize < 1 ? 1 : _fontSize;
                        text: _pitch
                        color: "white"
                        visible: (_pitch != 0) && ((_pitch % 10) === 0)
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
            origin.x: width  / 2
            origin.y: height / 2
            angle:    -rollAngle
            }
    ]
}

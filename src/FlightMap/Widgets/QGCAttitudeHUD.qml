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
import QGroundControl.ScreenTools 1.0

Item {
    id: root

    property bool active:       false  ///< true: actively connected to data provider, false: show inactive control
    property real rollAngle :   _defaultRollAngle
    property real pitchAngle:   _defaultPitchAngle
    property bool showPitch:    true

    readonly property real _defaultRollAngle:   0
    readonly property real _defaultPitchAngle:  0

    property real _rollAngle:   active ? rollAngle : _defaultRollAngle
    property real _pitchAngle:  active ? pitchAngle : _defaultPitchAngle

    anchors.centerIn: parent

    Image {
        id: rollDial
        anchors     { bottom: root.verticalCenter; horizontalCenter: parent.horizontalCenter}
        source:     "/qmlimages/rollDialWhite.svg"
        mipmap:     true
        width:      parent.width
        fillMode:   Image.PreserveAspectFit
        transform: Rotation {
            origin.x: rollDial.width / 2
            origin.y: rollDial.height
            angle:   -_rollAngle
        }
    }

    Image {
        id: pointer
        anchors     { bottom: root.verticalCenter; horizontalCenter: parent.horizontalCenter}
        source:     "/qmlimages/rollPointerWhite.svg"
        mipmap:     true
        width:      rollDial.width
        fillMode:   Image.PreserveAspectFit
    }

    Image {
        id:                 crossHair
        anchors.centerIn:   parent
        source:             "/qmlimages/crossHair.svg"
        mipmap:             true
        width:              parent.width
        fillMode:           Image.PreserveAspectFit
    }

    QGCPitchIndicator {
        id:                 pitchIndicator
        anchors.verticalCenter: parent.verticalCenter
        visible:            showPitch
        pitchAngle:         _pitchAngle
        rollAngle:          _rollAngle
        color:              Qt.rgba(0,0,0,0)
        size:               ScreenTools.defaultFontPixelSize * (10)
    }
}

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
 *   @brief QGC Current Altitude Indicator
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick 2.1
import QGroundControl.Controls 1.0
import QGroundControl.ScreenTools 1.0

Rectangle {
    id: root
    property real altitude: 0
    property real vertZ:    0
    property bool showAltitude: true
    property bool showClimbRate: true
    anchors.verticalCenter: parent.verticalCenter
    width:  parent.width
    height: (showAltitude && showClimbRate) ? ScreenTools.defaultFontPixelSize * (4.16) : ScreenTools.defaultFontPixelSize * (2.08)
    color: "black"
    border.color: Qt.rgba(1,1,1,0.25)
    opacity: 1.0
    Column{
        anchors.centerIn: parent
        spacing: ScreenTools.defaultFontPixelSize * (0.33)
        QGCLabel {
            text: 'h: ' + altitude.toFixed(0)
            font.weight: Font.DemiBold
            color: "white"
            anchors.right: parent.right
            visible: showAltitude
        }
        QGCLabel {
            text: 'vZ: ' + vertZ.toFixed(0)
            color: "white"
            font.weight: showAltitude ? Font.Normal : Font.DemiBold
            anchors.right: parent.right
            visible: showClimbRate
        }
    }
}

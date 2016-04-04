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

import QtQuick                      2.4
import QtQuick.Controls             1.3

import QGroundControl               1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controllers   1.0


Item {
    id: root
    Rectangle {
        id:             noVideo
        anchors.fill:   parent
        color:          Qt.rgba(0,0,0,0.75)
        visible:        !_controller.videoRunning
        QGCLabel {
            text:               qsTr("NO VIDEO")
            font.weight:        Font.DemiBold
            color:              "white"
            font.pixelSize:     _mainIsMap ? 12 * ScreenTools.fontHRatio : 20 * ScreenTools.fontHRatio
            anchors.centerIn:   parent
        }
    }
    QGCVideoBackground {
        anchors.fill:   parent
        display:        _controller.videoSurface
        receiver:       _controller.videoReceiver
        visible:        _controller.videoRunning
        runVideo:       true
        /* TODO: Come up with a way to make this an option
        QGCAttitudeHUD {
            id:                 attitudeHUD
            visible:            !_mainIsMap
            rollAngle:          _roll
            pitchAngle:         _pitch
            width:              ScreenTools.defaultFontPixelSize * (30)
            height:             ScreenTools.defaultFontPixelSize * (30)
            active:             QGroundControl.multiVehicleManager.activeVehicleAvailable
            z:                  QGroundControl.zOrderWidgets
        }
        */
    }
}

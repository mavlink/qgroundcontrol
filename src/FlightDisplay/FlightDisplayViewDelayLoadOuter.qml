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

import QtQuick                  2.4
import QtQuick.Controls         1.3
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.2
import QtLocation               5.3
import QtPositioning            5.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.FlightMap     1.0

/// This component is used to delay load the items which are direct children of the
/// FlightDisplayViewControl.
Item {
    QGCVideoBackground {
        anchors.fill:   parent
        display:        _controller.videoSurface
        receiver:       _controller.videoReceiver
        visible:        !_showMap

        QGCCompassHUD {
            id:                 compassHUD
            y:                  root.height * 0.7
            x:                  root.width  * 0.5 - ScreenTools.defaultFontPixelSize * (5)
            width:              ScreenTools.defaultFontPixelSize * (10)
            height:             ScreenTools.defaultFontPixelSize * (10)
            heading:            _heading
            active:             multiVehicleManager.activeVehicleAvailable
            z:                  _flightMap.zOrderWidgets
        }

        QGCAttitudeHUD {
            id:                 attitudeHUD
            rollAngle:          _roll
            pitchAngle:         _pitch
            width:              ScreenTools.defaultFontPixelSize * (30)
            height:             ScreenTools.defaultFontPixelSize * (30)
            active:             multiVehicleManager.activeVehicleAvailable
            z:                  _flightMap.zOrderWidgets
        }
    }

    QGCAltitudeWidget {
        anchors.right:  parent.right
        height:         parent.height * 0.65 > ScreenTools.defaultFontPixelSize * (23.4) ? ScreenTools.defaultFontPixelSize * (23.4) : parent.height * 0.65
        width:          ScreenTools.defaultFontPixelSize * (5)
        altitude:       _altitudeWGS84
        z:              _flightMap.zOrderWidgets
        visible:        !hideWidgets
    }

    QGCSpeedWidget {
        anchors.left:   parent.left
        width:          ScreenTools.defaultFontPixelSize * (5)
        height:         parent.height * 0.65 > ScreenTools.defaultFontPixelSize * (23.4) ? ScreenTools.defaultFontPixelSize * (23.4) : parent.height * 0.65
        speed:          _groundSpeed
        z:              _flightMap.zOrderWidgets
        visible:        !hideWidgets
    }

    QGCCurrentSpeed {
        anchors.left:       parent.left
        width:              ScreenTools.defaultFontPixelSize * (6.25)
        airspeed:           _airSpeed
        groundspeed:        _groundSpeed
        active:             multiVehicleManager.activeVehicleAvailable
        z:                  _flightMap.zOrderWidgets
        visible:             !hideWidgets
    }

    QGCCurrentAltitude {
        anchors.right:      parent.right
        width:              ScreenTools.defaultFontPixelSize * (6.25)
        altitude:           _altitudeWGS84
        vertZ:              _climbRate
        active:             multiVehicleManager.activeVehicleAvailable
        z:                  _flightMap.zOrderWidgets
        visible:              !hideWidgets
    }

    QGCButton {
        id:         optionsButton
        x:          _flightMap.mapWidgets.x
        y:          _flightMap.mapWidgets.y - height - (ScreenTools.defaultFontPixelHeight / 2)
        z:          _flightMap.zOrderWidgets
        width:      _flightMap.mapWidgets.width
        text:       "Options"
        menu:       optionsMenu
        visible:    _controller.hasVideo && !hideWidgets

        ExclusiveGroup {
            id: backgroundTypeGroup
        }

        Menu {
            id: optionsMenu

            MenuItem {
                id:             mapBackgroundMenuItem
                exclusiveGroup: backgroundTypeGroup
                checkable:      true
                checked:        _showMap
                text:           "Show map as background"

                onTriggered:    _setShowMap(true)
            }

            MenuItem {
                id:             videoBackgroundMenuItem
                exclusiveGroup: backgroundTypeGroup
                checkable:      true
                checked:        !_showMap
                text:           "Show video as background"

                onTriggered:    _setShowMap(false)
            }
        }
    }
}

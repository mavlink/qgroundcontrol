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

import QGroundControl.FlightMap     1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0


/// Flight Display View
Item {
    id: root

    property var __qgcPal: QGCPalette { colorGroupEnabled: enabled }

    property var _activeVehicle: multiVehicleManager.activeVehicle

    readonly property real _defaultLatitude:        37.803784
    readonly property real _defaultLongitude:       -122.462276
    readonly property real _defaultRoll:            0
    readonly property real _defaultPitch:           0
    readonly property real _defaultHeading:         0
    readonly property real _defaultAltitudeWGS84:   0
    readonly property real _defaultGroundSpeed:     0
    readonly property real _defaultAirSpeed:        0
    readonly property real _defaultClimbRate:       0

    property real _roll:            _activeVehicle ? (isNaN(_activeVehicle.roll) ? _defaultRoll : _activeVehicle.roll) : _defaultRoll
    property real _pitch:           _activeVehicle ? (isNaN(_activeVehicle.pitch) ? _defaultPitch : _activeVehicle.pitch) : _defaultPitch
    property real _latitude:        _activeVehicle ? ((_activeVehicle.latitude  === 0) ? _defaultLatitude : _activeVehicle.latitude) : _defaultLatitude
    property real _longitude:       _activeVehicle ? ((_activeVehicle.longitude === 0) ? _defaultLongitude : _activeVehicle.longitude) : _defaultLongitude
    property real _heading:         _activeVehicle ? (isNaN(_activeVehicle.heading) ? _defaultHeading : _activeVehicle.heading) : _defaultHeading
    property real _altitudeWGS84:   _activeVehicle ? _activeVehicle.altitudeWGS84 : _defaultAltitudeWGS84
    property real _groundSpeed:     _activeVehicle ? _activeVehicle.groundSpeed : _defaultGroundSpeed
    property real _airSpeed:        _activeVehicle ? _activeVehicle.airSpeed : _defaultAirSpeed
    property real _climbRate:       _activeVehicle ? _activeVehicle.climbRate : _defaultClimbRate

    function getBool(value) {
        return value === '0' ? false : true;
    }

    function setBool(value) {
        return value ? "1" : "0";
    }

    FlightMap {
        id:             flightMap
        anchors.fill:   parent
        mapName:        "FlightDisplayView"
        latitude:       _latitude
        longitude:      _longitude
        z:              10
        showVehicles:   true
    }

    QGCCompassWidget {
        x:          ScreenTools.defaultFontPixelSize * (7.1)
        y:          ScreenTools.defaultFontPixelSize * (0.42)
        size:       ScreenTools.defaultFontPixelSize * (13.3)
        heading:    _heading
        active:     multiVehicleManager.activeVehicleAvailable
        z:          flightMap.z + 2
    }

    QGCAttitudeWidget {
        anchors.rightMargin:    ScreenTools.defaultFontPixelSize * (7.1)
        anchors.right:          parent.right
        y:                      ScreenTools.defaultFontPixelSize * (0.42)
        size:                   ScreenTools.defaultFontPixelSize * (13.3)
        rollAngle:              _roll
        pitchAngle:             _pitch
        active:                 multiVehicleManager.activeVehicleAvailable
        z:                      flightMap.z + 2
    }

    QGCAltitudeWidget {
        anchors.right:  parent.right
        height:         parent.height * 0.65 > ScreenTools.defaultFontPixelSize * (23.4) ? ScreenTools.defaultFontPixelSize * (23.4) : parent.height * 0.65
        width:          ScreenTools.defaultFontPixelSize * (5)
        altitude:       _altitudeWGS84
        z:              30
    }

    QGCSpeedWidget {
        anchors.left:   parent.left
        width:          ScreenTools.defaultFontPixelSize * (5)
        height:         parent.height * 0.65 > ScreenTools.defaultFontPixelSize * (23.4) ? ScreenTools.defaultFontPixelSize * (23.4) : parent.height * 0.65
        speed:          _groundSpeed
        z:              40
    }

    QGCCurrentSpeed {
        anchors.left:       parent.left
        width:              ScreenTools.defaultFontPixelSize * (6.25)
        airspeed:           _airSpeed
        groundspeed:        _groundSpeed
        active:             multiVehicleManager.activeVehicleAvailable
        z:                  50
    }

    QGCCurrentAltitude {
        anchors.right:      parent.right
        width:              ScreenTools.defaultFontPixelSize * (6.25)
        altitude:           _altitudeWGS84
        vertZ:              _climbRate
        active:             multiVehicleManager.activeVehicleAvailable
        z:                  60
    }
}

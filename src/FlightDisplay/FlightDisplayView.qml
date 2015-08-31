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

    property var activeVehicle: multiVehicleManager.activeVehicle

    readonly property real defaultLatitude:         37.803784
    readonly property real defaultLongitude:        -122.462276
    readonly property real defaultRoll:             0
    readonly property real defaultPitch:            0
    readonly property real defaultHeading:          0
    readonly property real defaultAltitudeWGS84:    0
    readonly property real defaultGroundSpeed:      0
    readonly property real defaultAirSpeed:         0
    readonly property real defaultClimbRate:        0

    property real roll:             activeVehicle ? (isNaN(activeVehicle.roll) ? defaultRoll : activeVehicle.roll) : defaultRoll
    property real pitch:            activeVehicle ? (isNaN(activeVehicle.pitch) ? defaultPitch : activeVehicle.pitch) : defaultPitch
    property real latitude:         activeVehicle ? ((activeVehicle.latitude  === 0) ? defaultLatitude : activeVehicle.latitude) : defaultLatitude
    property real longitude:        activeVehicle ? ((activeVehicle.longitude === 0) ? defaultLongitude : activeVehicle.longitude) : defaultLongitude
    property real heading:          activeVehicle ? (isNaN(activeVehicle.heading) ? defaultHeading : activeVehicle.heading) : defaultHeading
    property real altitudeWGS84:    activeVehicle ? activeVehicle.altitudeWGS84 : defaultAltitudeWGS84
    property real groundSpeed:      activeVehicle ? activeVehicle.groundSpeed : defaultGroundSpeed
    property real airSpeed:         activeVehicle ? activeVehicle.airSpeed : defaultAirSpeed
    property real climbRate:        activeVehicle ? activeVehicle.climbRate : defaultClimbRate

    function getBool(value) {
        return value === '0' ? false : true;
    }

    function setBool(value) {
        return value ? "1" : "0";
    }

    VehicleTracker {
        map: mapBackground
    }

    FlightMap {
        id:                 mapBackground
        anchors.fill:       parent
        mapName:            'FlightDisplayView'
        latitude:           root.latitude
        longitude:          root.longitude
        readOnly:           true
        z:                  10
    }

    // Floating (Top Left) Compass Widget

    QGCCompassWidget {
        id:                 compassWidget
        y:                  ScreenTools.defaultFontPixelSize * (0.42)
        x:                  ScreenTools.defaultFontPixelSize * (7.1)
        size:               ScreenTools.defaultFontPixelSize * (13.3)
        heading:            root.heading
        active:             multiVehicleManager.activeVehicleAvailable
        z:                  mapBackground.z + 2
        onResetRequested: {
            y               = ScreenTools.defaultFontPixelSize * (0.42)
            x               = ScreenTools.defaultFontPixelSize * (7.1)
            size            = ScreenTools.defaultFontPixelSize * (13.3)
            tForm.xScale    = 1
            tForm.yScale    = 1
        }
    }

    // Floating (Top Right) Attitude Widget

    QGCAttitudeWidget {
        id:                 attitudeWidget
        y:                  ScreenTools.defaultFontPixelSize * (0.42)
        size:               ScreenTools.defaultFontPixelSize * (13.3)
        rollAngle:          roll
        pitchAngle:         pitch
        showPitch:          true
        anchors.right:      root.right
        anchors.rightMargin: ScreenTools.defaultFontPixelSize * (7.1)
        active:             multiVehicleManager.activeVehicleAvailable
        z:                  mapBackground.z + 2
        onResetRequested: {
            y                   = ScreenTools.defaultFontPixelSize * (0.42)
            anchors.right       = root.right
            anchors.rightMargin = ScreenTools.defaultFontPixelSize * (7.1)
            size                = ScreenTools.defaultFontPixelSize * (13.3)
            tForm.xScale        = 1
            tForm.yScale        = 1
        }
    }

    QGCAltitudeWidget {
        id:                 altitudeWidget
        anchors.right:      parent.right
        width:              ScreenTools.defaultFontPixelSize * (5)
        height:             parent.height * 0.65 > ScreenTools.defaultFontPixelSize * (23.4) ? ScreenTools.defaultFontPixelSize * (23.4) : parent.height * 0.65
        altitude:           root.altitudeWGS84
        z:                  30
    }

    QGCSpeedWidget {
        id:                 speedWidget
        anchors.left:       parent.left
        width:              ScreenTools.defaultFontPixelSize * (5)
        height:             parent.height * 0.65 > ScreenTools.defaultFontPixelSize * (23.4) ? ScreenTools.defaultFontPixelSize * (23.4) : parent.height * 0.65
        speed:              root.groundSpeed
        z:                  40
    }

    QGCCurrentSpeed {
        id: currentSpeed
        anchors.left:       parent.left
        width:              ScreenTools.defaultFontPixelSize * (6.25)
        airspeed:           root.airSpeed
        groundspeed:        root.groundSpeed
        active:             multiVehicleManager.activeVehicleAvailable
        showAirSpeed:       true
        showGroundSpeed:    true
        z:                  50
    }

    QGCCurrentAltitude {
        id: currentAltitude
        anchors.right:      parent.right
        width:              ScreenTools.defaultFontPixelSize * (6.25)
        altitude:           root.altitudeWGS84
        vertZ:              root.climbRate
        showAltitude:       true
        showClimbRate:      true
        active:             multiVehicleManager.activeVehicleAvailable
        z:                  60
    }
}

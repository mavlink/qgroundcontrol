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

import QGroundControl.FlightMap     1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Vehicle       1.0

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
    property real _heading:         _activeVehicle ? (isNaN(_activeVehicle.heading) ? _defaultHeading : _activeVehicle.heading) : _defaultHeading

    property real _latitude:        _activeVehicle ? ((_activeVehicle.latitude  === 0) ? _defaultLatitude : _activeVehicle.latitude) : _defaultLatitude
    property real _longitude:       _activeVehicle ? ((_activeVehicle.longitude === 0) ? _defaultLongitude : _activeVehicle.longitude) : _defaultLongitude

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
        id:                 flightMap
        anchors.fill:       parent
        mapName:            "FlightDisplayView"
        latitude:           parent._latitude
        longitude:          parent._longitude

        // Add the vehicles to the map
        MapItemView {
            model: multiVehicleManager.vehicles
            
            delegate:
                VehicleMapItem {
                        vehicle:        object
                        coordinate:     object.coordinate
                        isSatellite:    flightMap.isSatelliteMap
                }
        }

        // Add the mission items to the map
        MapItemView {
            model: multiVehicleManager.activeVehicle ? multiVehicleManager.activeVehicle.missionItems : 0
            
            delegate:
                MissionItemIndicator {
                    label:          object.sequenceNumber
                    isCurrentItem:  object.isCurrentItem
                    coordinate:     object.coordinate
                }
        }

        // Vehicle GPS lock display
        Column {
            id:     gpsLockColumn
            y:      (parent.height - height) / 2
            width:  parent.width

            Repeater {
                model: multiVehicleManager.vehicles
                
                delegate:
                    QGCLabel {
                        width:                  gpsLockColumn.width
                        horizontalAlignment:    Text.AlignHCenter
                        visible:                object.satelliteLock < 2
                        text:                   "No GPS Lock for Vehicle #" + object.id
                    }
            }
        }
    
        // Mission item list
        ListView {
            id:                 missionItemSummaryList
            anchors.margins:    ScreenTools.defaultFontPixelWidth
            anchors.left:       parent.left
            anchors.right:      flightMap.mapWidgets.left
            anchors.bottom:     parent.bottom
            height:             ScreenTools.defaultFontPixelHeight * 7
            spacing:            ScreenTools.defaultFontPixelWidth / 2
            opacity:            0.75
            orientation:        ListView.Horizontal
            model:              multiVehicleManager.activeVehicle ? multiVehicleManager.activeVehicle.missionItems : 0

            property real _maxItemHeight: 0

            delegate:
                MissionItemSummary {
                    opacity:        0.75
                    missionItem:    object
                }
        } // ListView - Mission item list

    }

    QGCCompassWidget {
        x:          ScreenTools.defaultFontPixelSize * (7.1)
        y:          ScreenTools.defaultFontPixelSize * (0.42)
        size:       ScreenTools.defaultFontPixelSize * (13.3)
        heading:    parent._heading
        active:     multiVehicleManager.activeVehicleAvailable
        z:          flightMap.z + 2
    }

    QGCAttitudeWidget {
        anchors.rightMargin:    ScreenTools.defaultFontPixelSize * (7.1)
        anchors.right:          parent.right
        y:                      ScreenTools.defaultFontPixelSize * (0.42)
        size:                   ScreenTools.defaultFontPixelSize * (13.3)
        rollAngle:              parent._roll
        pitchAngle:             parent._pitch
        active:                 multiVehicleManager.activeVehicleAvailable
        z:                      flightMap.z + 2
    }

    QGCAltitudeWidget {
        anchors.right:  parent.right
        height:         parent.height * 0.65 > ScreenTools.defaultFontPixelSize * (23.4) ? ScreenTools.defaultFontPixelSize * (23.4) : parent.height * 0.65
        width:          ScreenTools.defaultFontPixelSize * (5)
        altitude:       parent._altitudeWGS84
        z:              30
    }

    QGCSpeedWidget {
        anchors.left:   parent.left
        width:          ScreenTools.defaultFontPixelSize * (5)
        height:         parent.height * 0.65 > ScreenTools.defaultFontPixelSize * (23.4) ? ScreenTools.defaultFontPixelSize * (23.4) : parent.height * 0.65
        speed:          parent._groundSpeed
        z:              40
    }

    QGCCurrentSpeed {
        anchors.left:       parent.left
        width:              ScreenTools.defaultFontPixelSize * (6.25)
        airspeed:           parent._airSpeed
        groundspeed:        parent._groundSpeed
        active:             multiVehicleManager.activeVehicleAvailable
        z:                  50
    }

    QGCCurrentAltitude {
        anchors.right:      parent.right
        width:              ScreenTools.defaultFontPixelSize * (6.25)
        altitude:           parent._altitudeWGS84
        vertZ:              parent._climbRate
        active:             multiVehicleManager.activeVehicleAvailable
        z:                  60
    }
}

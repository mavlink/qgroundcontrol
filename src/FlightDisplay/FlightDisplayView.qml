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
import QGroundControl.FlightDisplay 1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controllers   1.0

/// Flight Display View
Item {
    id: root

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    property var _activeVehicle: multiVehicleManager.activeVehicle

    readonly property var  _defaultVehicleCoordinate:   QtPositioning.coordinate(37.803784, -122.462276)
    readonly property real _defaultRoll:                0
    readonly property real _defaultPitch:               0
    readonly property real _defaultHeading:             0
    readonly property real _defaultAltitudeWGS84:       0
    readonly property real _defaultGroundSpeed:         0
    readonly property real _defaultAirSpeed:            0
    readonly property real _defaultClimbRate:           0

    readonly property string _mapName:                  "FlightDisplayView"
    readonly property string _showMapBackgroundKey:     "/showMapBackground"

    property bool _mainIsMap:           !_controller.hasVideo

    property real _roll:                _activeVehicle ? (isNaN(_activeVehicle.roll)    ? _defaultRoll    : _activeVehicle.roll)    : _defaultRoll
    property real _pitch:               _activeVehicle ? (isNaN(_activeVehicle.pitch)   ? _defaultPitch   : _activeVehicle.pitch)   : _defaultPitch
    property real _heading:             _activeVehicle ? (isNaN(_activeVehicle.heading) ? _defaultHeading : _activeVehicle.heading) : _defaultHeading

    property var  _vehicleCoordinate:   _activeVehicle ? (_activeVehicle.coordinateValid ? _activeVehicle.coordinate : _defaultVehicleCoordinate) : _defaultVehicleCoordinate

    property real _altitudeWGS84:       _activeVehicle ? _activeVehicle.altitudeWGS84 : _defaultAltitudeWGS84
    property real _groundSpeed:         _activeVehicle ? _activeVehicle.groundSpeed   : _defaultGroundSpeed
    property real _airSpeed:            _activeVehicle ? _activeVehicle.airSpeed      : _defaultAirSpeed
    property real _climbRate:           _activeVehicle ? _activeVehicle.climbRate     : _defaultClimbRate

    property var  _flightMap:           null
    property var  _flightVideo:         null
    property var  _savedZoomLevel:      0

    FlightDisplayViewController { id: _controller }

    MissionController {
        id: _missionController
        Component.onCompleted: start(false /* editMode */)
    }

    function reloadContents() {
        if(_flightVideo) {
            _flightVideo.visible = false
        }
        if(_mainIsMap) {
            mainLoader.source   = "FlightDisplayViewMap.qml"
            pipLoader.source    = "FlightDisplayViewVideo.qml"
        } else {
            mainLoader.source   = "FlightDisplayViewVideo.qml"
            pipLoader.source    = "FlightDisplayViewMap.qml"
        }
    }

    Component.onCompleted: {
        reloadContents();
        widgetsLoader.source    = "FlightDisplayViewWidgets.qml"
    }

    //-- Main Window
    Loader {
        id:                 mainLoader
        anchors.fill:       parent
        onLoaded: {
            if(_mainIsMap) {
                _flightMap   = item
                if(_savedZoomLevel != 0)
                    _flightMap.zoomLevel = _savedZoomLevel
                else
                    _savedZoomLevel = _flightMap.zoomLevel
                _flightMap.updateMapPosition(true /* force */)
            } else {
                _flightVideo = item
                _flightVideo.visible = true
            }
        }
    }

    //-- PIP Window
    Rectangle {
        id:                 pip
        visible:            _controller.hasVideo
        anchors.margins:    ScreenTools.defaultFontPixelHeight
        anchors.left:       parent.left
        anchors.bottom:     parent.bottom
        height:             ScreenTools.defaultFontPixelSize * (9)
        width:              ScreenTools.defaultFontPixelSize * (9) * (16/9)
        color:              "#000010"
        border.width:       4
        radius:             4
        border.color: {
            if(_mainIsMap && _flightMap != null)
                return _flightMap.isSatelliteMap ? Qt.rgba(1,1,1,0.75) :  Qt.rgba(0,0,0,0.75)
            else
                return Qt.rgba(0,0,0,0.75)
        }
        Loader {
            id:                 pipLoader
            anchors.fill:       parent
            anchors.margins:    2
            onLoaded: {
                if(_mainIsMap) {
                    _flightVideo = item
                    _flightVideo.visible = true
                } else {
                    _flightMap = item
                    _savedZoomLevel = _flightMap.zoomLevel
                    _flightMap.zoomLevel = _savedZoomLevel - 3
                }
                pip.visible = _controller.hasVideo
            }
        }
        MouseArea {
            anchors.fill: parent
            onClicked: {
                _mainIsMap = !_mainIsMap
                pip.visible = false
                reloadContents();
            }
        }
    }

    //-- Widgets
    Loader {
        id:                 widgetsLoader
        anchors.fill:       parent
    }
}

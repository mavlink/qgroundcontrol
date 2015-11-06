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

    property real avaiableHeight: parent.height

    readonly property bool isBackgroundDark: _mainIsMap ? (_flightMap ? _flightMap.isSatelliteMap : true) : true

    property var _activeVehicle:  multiVehicleManager.activeVehicle

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
    readonly property string _mainIsMapKey:             "MainFlyWindowIsMap"
    readonly property string _PIPVisibleKey:            "IsPIPVisible"

    property bool _mainIsMap:           QGroundControl.loadBoolGlobalSetting(_mainIsMapKey,  true)
    property bool _isPipVisible:        QGroundControl.loadBoolGlobalSetting(_PIPVisibleKey, true)

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

    property real _pipSize:             mainWindow.width * 0.2

    FlightDisplayViewController { id: _controller }

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
            }
        }
    }

    //-- PIP Window
    Rectangle {
        id:                 pip
        visible:            _controller.hasVideo && _isPipVisible
        anchors.margins:    ScreenTools.defaultFontPixelHeight
        anchors.left:       parent.left
        anchors.bottom:     parent.bottom
        width:              _pipSize
        height:             _pipSize * (9/16)
        color:              "#000010"
        border.color:       isBackgroundDark ? Qt.rgba(1,1,1,0.75) : Qt.rgba(0,0,0,0.75)
        Loader {
            id:                 pipLoader
            anchors.fill:       parent
            onLoaded: {
                if(_mainIsMap) {
                    _flightVideo = item
                } else {
                    _flightMap = item
                    _savedZoomLevel = _flightMap.zoomLevel
                    _flightMap.zoomLevel = _savedZoomLevel - 3
                }
            }
        }
        MouseArea {
            anchors.fill: parent
            onClicked: {
                _mainIsMap = !_mainIsMap
                reloadContents();
                QGroundControl.saveBoolGlobalSetting(_mainIsMapKey, _mainIsMap)
            }
        }
        Image {
            id:             closePIP
            source:         "/qmlimages/PiP.svg"
            mipmap:         true
            fillMode:       Image.PreserveAspectFit
            anchors.left:   parent.left
            anchors.bottom: parent.bottom
            height:         ScreenTools.defaultFontPixelSize * 2.5
            width:          ScreenTools.defaultFontPixelSize * 2.5
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    _isPipVisible = false
                    QGroundControl.saveBoolGlobalSetting(_PIPVisibleKey, false)
                }
            }
        }
    }

    //-- Show PIP
    Rectangle {
        id:                     openPIP
        anchors.left :          parent.left
        anchors.bottom:         parent.bottom
        anchors.margins:        ScreenTools.defaultFontPixelHeight
        height:                 ScreenTools.defaultFontPixelSize * 2
        width:                  ScreenTools.defaultFontPixelSize * 2
        radius:                 ScreenTools.defaultFontPixelSize / 3
        visible:                _controller.hasVideo && !_isPipVisible
        color:                  isBackgroundDark ? Qt.rgba(0,0,0,0.75) : Qt.rgba(0,0,0,0.5)
        Image {
            width:              parent.width  * 0.75
            height:             parent.height * 0.75
            source:             "/res/buttonRight.svg"
            mipmap:             true
            fillMode:           Image.PreserveAspectFit
            anchors.verticalCenter:     parent.verticalCenter
            anchors.horizontalCenter:   parent.horizontalCenter
        }
        MouseArea {
            anchors.fill: parent
            onClicked: {
                _isPipVisible = true
                QGroundControl.saveBoolGlobalSetting(_PIPVisibleKey, true)
            }
        }
    }

    //-- Widgets
    Loader {
        id:                 widgetsLoader
        anchors.right:      parent.right
        anchors.left:       parent.left
        anchors.bottom:     parent.bottom
        height:             avaiableHeight
    }

}

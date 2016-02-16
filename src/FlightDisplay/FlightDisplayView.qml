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

import QtQuick                  2.5
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
import QGroundControl.FactSystem    1.0

/// Flight Display View
QGCView {
    id:             root
    viewPanel:      _panel
    topDialogMargin: height - availableHeight

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    property real availableHeight: parent.height

    readonly property bool isBackgroundDark: _mainIsMap ? (_flightMap ? _flightMap.isSatelliteMap : true) : true

    property var _activeVehicle:    multiVehicleManager.activeVehicle

    readonly property real _defaultRoll:                0
    readonly property real _defaultPitch:               0
    readonly property real _defaultHeading:             0
    readonly property real _defaultAltitudeAMSL:        0
    readonly property real _defaultGroundSpeed:         0
    readonly property real _defaultAirSpeed:            0

    readonly property string _mapName:                  "FlightDisplayView"
    readonly property string _showMapBackgroundKey:     "/showMapBackground"
    readonly property string _mainIsMapKey:             "MainFlyWindowIsMap"
    readonly property string _PIPVisibleKey:            "IsPIPVisible"

    property bool _mainIsMap:           QGroundControl.loadBoolGlobalSetting(_mainIsMapKey,  true)
    property bool _isPipVisible:        QGroundControl.loadBoolGlobalSetting(_PIPVisibleKey, true)

    property real _roll:                _activeVehicle ? _activeVehicle.roll.value    : _defaultRoll
    property real _pitch:               _activeVehicle ? _activeVehicle.pitch.value   : _defaultPitch
    property real _heading:             _activeVehicle ? _activeVehicle.heading.value : _defaultHeading


    property Fact _emptyFact:               Fact { }
    property Fact _groundSpeedFact:         _activeVehicle ? _activeVehicle.groundSpeed      : _emptyFact
    property Fact _airSpeedFact:            _activeVehicle ? _activeVehicle.airSpeed         : _emptyFact

    property bool activeVehicleJoystickEnabled: _activeVehicle ? _activeVehicle.joystickEnabled : false

    property real _savedZoomLevel:      0

    property real pipSize:              mainWindow.width * 0.2

    FlightDisplayViewController { id: _controller }

    function setStates() {
        if(_mainIsMap) {
            //-- Adjust Margins
            _flightMapContainer.state   = "fullMode"
            _flightVideo.state          = "pipMode"
            //-- Save/Restore Map Zoom Level
            if(_savedZoomLevel != 0)
                _flightMap.zoomLevel = _savedZoomLevel
            else
                _savedZoomLevel = _flightMap.zoomLevel
        } else {
            //-- Adjust Margins
            _flightMapContainer.state   = "pipMode"
            _flightVideo.state          = "fullMode"
            //-- Set Map Zoom Level
            _savedZoomLevel = _flightMap.zoomLevel
            _flightMap.zoomLevel = _savedZoomLevel - 3
        }
    }

    function setPipVisibility(state) {
        _isPipVisible = state;
        QGroundControl.saveBoolGlobalSetting(_PIPVisibleKey, state)
    }

    function px4JoystickCheck() {
        if (_activeVehicle && !_activeVehicle.px4Firmware && (QGroundControl.virtualTabletJoystick || _activeVehicle.joystickEnabled)) {
            px4JoystickSupport.open()
        }
    }

    MessageDialog {
        id:     px4JoystickSupport
        text:   "Joystick support requires MAVLink MANUAL_CONTROL support. " +
                "The firmware you are running does not normally support this. " +
                "It will only work if you have modified the firmware to add MANUAL_CONTROL support."
    }

    Connections {
        target: multiVehicleManager
        onActiveVehicleChanged: px4JoystickCheck()
    }

    Connections {
        target: QGroundControl
        onVirtualTabletJoystickChanged: px4JoystickCheck()
    }

    onActiveVehicleJoystickEnabledChanged: px4JoystickCheck()

    Component.onCompleted: {
        widgetsLoader.source = "FlightDisplayViewWidgets.qml"
        setStates()
        px4JoystickCheck()
    }

    QGCViewPanel {
        id:             _panel
        anchors.fill:   parent

        //-- Map View
        //   For whatever reason, if FlightDisplayViewMap is the _panel item, changing
        //   width/height has no effect.
        Item {
            id: _flightMapContainer
            z:  _mainIsMap ? _panel.z + 1 : _panel.z + 2
            anchors.left:   _panel.left
            anchors.bottom: _panel.bottom
            visible:        _mainIsMap || _isPipVisible
            width:          _mainIsMap ? _panel.width  : pipSize
            height:         _mainIsMap ? _panel.height : pipSize * (9/16)
            states: [
                State {
                    name:   "pipMode"
                    PropertyChanges {
                        target:             _flightMapContainer
                        anchors.margins:    ScreenTools.defaultFontPixelHeight
                    }
                },
                State {
                    name:   "fullMode"
                    PropertyChanges {
                        target:             _flightMapContainer
                        anchors.margins:    0
                    }
                }
            ]
            FlightDisplayViewMap {
                id:             _flightMap
                anchors.fill:   parent
            }
        }

        //-- Video View
        FlightDisplayViewVideo {
            id:             _flightVideo
            z:              _mainIsMap ? _panel.z + 2 : _panel.z + 1
            width:          !_mainIsMap ? _panel.width  : pipSize
            height:         !_mainIsMap ? _panel.height : pipSize * (9/16)
            anchors.left:   _panel.left
            anchors.bottom: _panel.bottom
            visible:        _controller.hasVideo && (!_mainIsMap || _isPipVisible)
            states: [
                State {
                    name:   "pipMode"
                    PropertyChanges {
                        target: _flightVideo
                        anchors.margins:    ScreenTools.defaultFontPixelHeight
                    }
                },
                State {
                    name:   "fullMode"
                    PropertyChanges {
                        target: _flightVideo
                        anchors.margins:    0
                    }
                }
            ]
        }

        QGCPipable {
            id:                 _flightVideoPipControl
            z:                  _flightVideo.z + 3
            width:              pipSize
            height:             pipSize * (9/16)
            anchors.left:       _panel.left
            anchors.bottom:     _panel.bottom
            anchors.margins:    ScreenTools.defaultFontPixelHeight
            isHidden:           !_isPipVisible
            isDark:             isBackgroundDark
            onActivated: {
                _mainIsMap = !_mainIsMap
                setStates()
            }
            onHideIt: {
                setPipVisibility(!state)
            }
        }

        //-- Widgets
        Loader {
            id:                 widgetsLoader
            z:                  _panel.z + 4
            anchors.right:      parent.right
            anchors.left:       parent.left
            anchors.bottom:     parent.bottom
            height:             availableHeight
            property bool isBackgroundDark: root.isBackgroundDark
            property var qgcView: root
        }

        //-- Virtual Joystick
        Item {
            id:                         multiTouchItem
            z:                          _panel.z + 5
            width:                      parent.width  - (_flightVideoPipControl.width / 2)
            height:                     thumbAreaHeight
            visible:                    QGroundControl.virtualTabletJoystick
            anchors.bottom:             _flightVideoPipControl.top
            anchors.bottomMargin:       ScreenTools.defaultFontPixelHeight * 2
            anchors.horizontalCenter:   parent.horizontalCenter

            readonly property real thumbAreaHeight: Math.min(parent.height * 0.25, ScreenTools.defaultFontPixelWidth * 16)

            QGCMapPalette { id: mapPal; lightColors: !isBackgroundDark }

            Timer {
                interval:   40  // 25Hz, same as real joystick rate
                running:    QGroundControl.virtualTabletJoystick && _activeVehicle
                repeat:     true
                onTriggered: {
                    if (_activeVehicle) {
                        _activeVehicle.virtualTabletJoystickValue(rightStick.xAxis, rightStick.yAxis, leftStick.xAxis, leftStick.yAxis)
                    }
                }
            }

            JoystickThumbPad {
                id:                     leftStick
                anchors.leftMargin:     xPositionDelta
                anchors.bottomMargin:   -yPositionDelta
                anchors.left:           parent.left
                anchors.bottom:         parent.bottom
                width:                  parent.thumbAreaHeight
                height:                 parent.thumbAreaHeight
                yAxisThrottle:          true
                lightColors:            !isBackgroundDark
            }

            JoystickThumbPad {
                id:                     rightStick
                anchors.rightMargin:    -xPositionDelta
                anchors.bottomMargin:   -yPositionDelta
                anchors.right:          parent.right
                anchors.bottom:         parent.bottom
                width:                  parent.thumbAreaHeight
                height:                 parent.thumbAreaHeight
                lightColors:            !isBackgroundDark
            }
        }
    }
}

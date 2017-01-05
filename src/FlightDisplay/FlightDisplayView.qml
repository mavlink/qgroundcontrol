/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                  2.5
import QtQuick.Controls         1.3
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.2
import QtLocation               5.3
import QtPositioning            5.2
import QtMultimedia             5.5

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

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    property var _activeVehicle:        QGroundControl.multiVehicleManager.activeVehicle
    property bool _mainIsMap:           QGroundControl.videoManager.hasVideo ? QGroundControl.loadBoolGlobalSetting(_mainIsMapKey,  true) : true
    property bool _isPipVisible:        QGroundControl.videoManager.hasVideo ? QGroundControl.loadBoolGlobalSetting(_PIPVisibleKey, true) : false

    property real _roll:                _activeVehicle ? _activeVehicle.roll.value    : _defaultRoll
    property real _pitch:               _activeVehicle ? _activeVehicle.pitch.value   : _defaultPitch
    property real _heading:             _activeVehicle ? _activeVehicle.heading.value : _defaultHeading

    property Fact _emptyFact:               Fact { }
    property Fact _groundSpeedFact:         _activeVehicle ? _activeVehicle.groundSpeed      : _emptyFact
    property Fact _airSpeedFact:            _activeVehicle ? _activeVehicle.airSpeed         : _emptyFact

    property bool activeVehicleJoystickEnabled: _activeVehicle ? _activeVehicle.joystickEnabled : false

    property real _savedZoomLevel:  0
    property real _margins:         ScreenTools.defaultFontPixelWidth / 2


    property real pipSize:              mainWindow.width * 0.2

    readonly property bool      isBackgroundDark:       _mainIsMap ? (_flightMap ? _flightMap.isSatelliteMap : true) : true
    readonly property real      _defaultRoll:           0
    readonly property real      _defaultPitch:          0
    readonly property real      _defaultHeading:        0
    readonly property real      _defaultAltitudeAMSL:   0
    readonly property real      _defaultGroundSpeed:    0
    readonly property real      _defaultAirSpeed:       0
    readonly property string    _mapName:               "FlightDisplayView"
    readonly property string    _showMapBackgroundKey:  "/showMapBackground"
    readonly property string    _mainIsMapKey:          "MainFlyWindowIsMap"
    readonly property string    _PIPVisibleKey:         "IsPIPVisible"

    function setStates() {
        QGroundControl.saveBoolGlobalSetting(_mainIsMapKey, _mainIsMap)
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
        if ( _activeVehicle && !_activeVehicle.supportsManualControl && (QGroundControl.virtualTabletJoystick || _activeVehicle.joystickEnabled)) {
            px4JoystickSupport.open()
        }
    }

    MessageDialog {
        id:     px4JoystickSupport
        text:   qsTr("Joystick support requires MAVLink MANUAL_CONTROL support. ") +
                qsTr("The firmware you are running does not normally support this. ") +
                qsTr("It will only work if you have modified the firmware to add MANUAL_CONTROL support.")
    }

    Connections {
        target: QGroundControl.multiVehicleManager
        onActiveVehicleChanged: px4JoystickCheck()
    }

    Connections {
        target: QGroundControl
        onVirtualTabletJoystickChanged: px4JoystickCheck()
    }

    onActiveVehicleJoystickEnabledChanged: px4JoystickCheck()

    Component.onCompleted: {
        setStates()
        px4JoystickCheck()
    }

    QGCMapPalette { id: mapPal; lightColors: _mainIsMap ? _flightMap.isSatelliteMap : true }

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
                id:                 _flightMap
                anchors.fill:       parent
                flightWidgets:      flightDisplayViewWidgets
                rightPanelWidth:    ScreenTools.defaultFontPixelHeight * 9
            }
        }

        //-- Video View
        Item {
            id:             _flightVideo
            z:              _mainIsMap ? _panel.z + 2 : _panel.z + 1
            width:          !_mainIsMap ? _panel.width  : pipSize
            height:         !_mainIsMap ? _panel.height : pipSize * (9/16)
            anchors.left:   _panel.left
            anchors.bottom: _panel.bottom
            visible:        QGroundControl.videoManager.hasVideo && (!_mainIsMap || _isPipVisible)
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
            //-- Video Streaming
            FlightDisplayViewVideo {
                anchors.fill:   parent
                visible:        QGroundControl.videoManager.isGStreamer
            }
            //-- UVC Video (USB Camera or Video Device)
            Loader {
                id:             cameraLoader
                anchors.fill:   parent
                visible:        !QGroundControl.videoManager.isGStreamer
                source:         QGroundControl.videoManager.uvcEnabled ? "qrc:/qml/FlightDisplayViewUVC.qml" : "qrc:/qml/FlightDisplayViewDummy.qml"
            }
        }

        QGCPipable {
            id:                 _flightVideoPipControl
            z:                  _flightVideo.z + 3
            width:              pipSize
            height:             pipSize * (9/16)
            anchors.left:       _panel.left
            anchors.bottom:     _panel.bottom
            anchors.margins:    ScreenTools.defaultFontPixelHeight
            visible:            QGroundControl.videoManager.hasVideo
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

        Row {
            id:                     singleMultiSelector
            anchors.topMargin:      ScreenTools.toolbarHeight + _margins
            anchors.rightMargin:    _margins
            anchors.right:          parent.right
            anchors.top:            parent.top
            spacing:                ScreenTools.defaultFontPixelWidth
            z:                      _panel.z + 4
            visible:                QGroundControl.multiVehicleManager.vehicles.count > 1

            ExclusiveGroup { id: multiVehicleSelectorGroup }

            QGCRadioButton {
                id:             singleVehicleView
                exclusiveGroup: multiVehicleSelectorGroup
                text:           qsTr("Single")
                checked:        true
                color:          mapPal.text
            }

            QGCRadioButton {
                exclusiveGroup: multiVehicleSelectorGroup
                text:           qsTr("Multi-Vehicle (WIP)")
                color:          mapPal.text
            }
        }

        FlightDisplayViewWidgets {
            id:                 flightDisplayViewWidgets
            z:                  _panel.z + 4
            height:             ScreenTools.availableHeight
            anchors.left:       parent.left
            anchors.right:      parent.right
            anchors.bottom:     parent.bottom
            qgcView:            root
            isBackgroundDark:   root.isBackgroundDark
            visible:            singleVehicleView.checked
        }

        MultiVehicleList {
            anchors.margins:    _margins
            anchors.top:        singleMultiSelector.bottom
            anchors.right:      parent.right
            anchors.bottom:     parent.bottom
            width:              ScreenTools.defaultFontPixelWidth * 30
            visible:            !singleVehicleView.checked
            z:                  _panel.z + 4
        }


        //-- Virtual Joystick
        Loader {
            id:                         virtualJoystickMultiTouch
            z:                          _panel.z + 5
            width:                      parent.width  - (_flightVideoPipControl.width / 2)
            height:                     Math.min(ScreenTools.availableHeight * 0.25, ScreenTools.defaultFontPixelWidth * 16)
            visible:                    QGroundControl.virtualTabletJoystick
            anchors.bottom:             _flightVideoPipControl.top
            anchors.bottomMargin:       ScreenTools.defaultFontPixelHeight * 2
            anchors.horizontalCenter:   flightDisplayViewWidgets.horizontalCenter
            source:                     "qrc:/qml/VirtualJoystick.qml"
            active:                     QGroundControl.virtualTabletJoystick

            property bool useLightColors: root.isBackgroundDark
        }
    }
}

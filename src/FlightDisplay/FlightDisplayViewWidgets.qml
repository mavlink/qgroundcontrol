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

Item {
    id: _root

    property alias guidedModeBar: _guidedModeBar

    property var    _activeVehicle:         QGroundControl.multiVehicleManager.activeVehicle
    property bool   _isSatellite:   _mainIsMap ? _flightMap ? _flightMap.isSatelliteMap : true : true

    readonly property real _margins: ScreenTools.defaultFontPixelHeight / 2

    QGCMapPalette { id: mapPal; lightColors: !isBackgroundDark }

    function getGadgetWidth() {
        if(ScreenTools.isMobile) {
            if(ScreenTools.isTinyScreen)
                return mainWindow.width * 0.2
            return mainWindow.width * 0.15
        }
        var w = mainWindow.width * 0.15
        return Math.min(w, 200)
    }

    ExclusiveGroup {
        id: _dropButtonsExclusiveGroup
    }

    //-- Vehicle GPS lock display
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
                visible:                !object.coordinateValid
                text:                   "No GPS Lock for Vehicle #" + object.id
                z:                      QGroundControl.zOrderMapItems - 2
                color:                  mapPal.text
            }
        }
    }

    //-- Dismiss Drop Down (if any)
    MouseArea {
        anchors.fill:   parent
        enabled:        _dropButtonsExclusiveGroup.current != null
        onClicked: {
            if(_dropButtonsExclusiveGroup.current)
                _dropButtonsExclusiveGroup.current.checked = false
            _dropButtonsExclusiveGroup.current = null
        }
    }

    //-- Instrument Panel
    QGCInstrumentWidget {
        id:                     instrumentGadget
        anchors.margins:        ScreenTools.defaultFontPixelHeight / 2
        anchors.right:          altitudeSlider.visible ? altitudeSlider.left : parent.right
        anchors.verticalCenter: parent.verticalCenter
        visible:                !QGroundControl.virtualTabletJoystick
        size:                   getGadgetWidth()
        active:                 _activeVehicle != null
        heading:                _heading
        rollAngle:              _roll
        pitchAngle:             _pitch
        groundSpeedFact:        _groundSpeedFact
        airSpeedFact:           _airSpeedFact
        isSatellite:            _isSatellite
        z:                      QGroundControl.zOrderWidgets
        qgcView:                parent.parent.qgcView
        maxHeight:              parent.height - (anchors.margins * 2)
    }

    QGCInstrumentWidgetAlternate {
        id:                     instrumentGadgetAlternate
        anchors.margins:        ScreenTools.defaultFontPixelHeight / 2
        anchors.top:            parent.top
        anchors.right:          altitudeSlider.visible ? altitudeSlider.left : parent.right
        visible:                QGroundControl.virtualTabletJoystick
        width:                  getGadgetWidth()
        active:                 _activeVehicle != null
        heading:                _heading
        rollAngle:              _roll
        pitchAngle:             _pitch
        groundSpeedFact:        _groundSpeedFact
        airSpeedFact:           _airSpeedFact
        isSatellite:            _isSatellite
        z:                      QGroundControl.zOrderWidgets
    }

    ValuesWidget {
        anchors.topMargin:  ScreenTools.defaultFontPixelHeight
        anchors.top:        instrumentGadgetAlternate.bottom
        anchors.left:       instrumentGadgetAlternate.left
        width:              getGadgetWidth()
        qgcView:            parent.parent.qgcView
        textColor:          _isSatellite ? "white" : "black"
        visible:            QGroundControl.virtualTabletJoystick
        maxHeight:          multiTouchItem.y - y
    }

    //-- Vertical Tool Buttons
    Column {
        id:                         toolColumn
        visible:                    _mainIsMap
        anchors.margins:            ScreenTools.defaultFontPixelHeight
        anchors.left:               parent.left
        anchors.top:                parent.top
        spacing:                    ScreenTools.defaultFontPixelHeight

        //-- Map Center Control
        DropButton {
            id:                     centerMapDropButton
            dropDirection:          dropRight
            buttonImage:            "/qmlimages/MapCenter.svg"
            viewportMargins:        ScreenTools.defaultFontPixelWidth / 2
            exclusiveGroup:         _dropButtonsExclusiveGroup
            z:                      QGroundControl.zOrderWidgets

            dropDownComponent: Component {
                Row {
                    spacing: ScreenTools.defaultFontPixelWidth

                    QGCCheckBox {
                        id:                 followVehicleCheckBox
                        text:               "Follow Vehicle"
                        checked:            _flightMap ? _flightMap._followVehicle : false
                        anchors.baseline:   centerMapButton.baseline

                        onClicked: {
                            _dropButtonsExclusiveGroup.current = null
                            _flightMap._followVehicle = !_flightMap._followVehicle
                        }
                    }

                    QGCButton {
                        id:         centerMapButton
                        text:       "Center map on Vehicle"
                        enabled:    _activeVehicle && !followVehicleCheckBox.checked

                        property var activeVehicle: multiVehicleManager.activeVehicle

                        onClicked: {
                            _dropButtonsExclusiveGroup.current = null
                            _flightMap.center = activeVehicle.coordinate
                        }
                    }
                }
            }
        }

        //-- Map Type Control
        DropButton {
            id:                     mapTypeButton
            dropDirection:          dropRight
            buttonImage:            "/qmlimages/MapType.svg"
            viewportMargins:        ScreenTools.defaultFontPixelWidth / 2
            exclusiveGroup:         _dropButtonsExclusiveGroup
            z:                      QGroundControl.zOrderWidgets

            dropDownComponent: Component {
                Column {
                    spacing: ScreenTools.defaultFontPixelWidth

                    Row {
                        spacing: ScreenTools.defaultFontPixelWidth

                        Repeater {
                            model: QGroundControl.flightMapSettings.mapTypes

                            QGCButton {
                                checkable:  true
                                checked:    _flightMap ? _flightMap.mapType == text : false
                                text:       modelData

                                onClicked: {
                                    _flightMap.mapType = text
                                    _dropButtonsExclusiveGroup.current = null
                                }
                            }
                        }
                    }

                    QGCButton {
                        text:       "Clear flight trails"
                        enabled:    QGroundControl.multiVehicleManager.activeVehicle

                        onClicked: {
                            QGroundControl.multiVehicleManager.activeVehicle.clearTrajectoryPoints()
                            _dropButtonsExclusiveGroup.current = null
                        }
                    }
                }
            }
        }

        //-- Zoom Map In
        RoundButton {
            id:                 mapZoomPlus
            visible:            _mainIsMap && !ScreenTools.isTinyScreen
            buttonImage:        "/qmlimages/ZoomPlus.svg"
            exclusiveGroup:     _dropButtonsExclusiveGroup
            z:                  QGroundControl.zOrderWidgets
            onClicked: {
                if(_flightMap)
                    _flightMap.zoomLevel += 0.5
                checked = false
            }
        }

        //-- Zoom Map Out
        RoundButton {
            id:                 mapZoomMinus
            visible:            _mainIsMap && !ScreenTools.isTinyScreen
            buttonImage:        "/qmlimages/ZoomMinus.svg"
            exclusiveGroup:     _dropButtonsExclusiveGroup
            z:                  QGroundControl.zOrderWidgets
            onClicked: {
                if(_flightMap)
                    _flightMap.zoomLevel -= 0.5
                checked = false
            }
        }
    }

    //-- Guided mode buttons
    Rectangle {
        id:                         _guidedModeBar
        anchors.margins:            _margins
        anchors.bottom:             parent.bottom
        anchors.horizontalCenter:   parent.horizontalCenter
        width:                      (guidedModeButtons.visible ? guidedModeButtons.width : guidedModeConfirm.width) + (_margins * 2)
        height:                     (guidedModeButtons.visible ? guidedModeButtons.height : guidedModeConfirm.height) + (_margins * 2)
        color:                      qgcPal.window
        visible:                    _activeVehicle
        opacity:                    0.9
        z:                          QGroundControl.zOrderWidgets

        readonly property int confirmHome:          1
        readonly property int confirmLand:          2
        readonly property int confirmTakeoff:       3
        readonly property int confirmArm:           4
        readonly property int confirmDisarm:        5
        readonly property int confirmEmergencyStop: 6
        readonly property int confirmChangeAlt:     7
        readonly property int confirmGoTo:          8

        property int    confirmActionCode
        property string confirmText

        function actionConfirmed() {
            switch (confirmActionCode) {
            case confirmHome:
                _activeVehicle.guidedModeRTL()
                break;
            case confirmLand:
                _activeVehicle.guidedModeLand()
                break;
            case confirmTakeoff:
                var altitude = altitudeSlider.getValue()
                if (!isNaN(altitude)) {
                    _activeVehicle.guidedModeTakeoff(altitude)
                }
                break;
            case confirmArm:
                _activeVehicle.armed = true
                break;
            case confirmDisarm:
                _activeVehicle.armed = false
                break;
            case confirmEmergencyStop:
                _activeVehicle.emergencyStop()
                break;
            case confirmChangeAlt:
                var altitude = altitudeSlider.getValue()
                if (!isNaN(altitude)) {
                    _activeVehicle.guidedModeChangeAltitude(altitude)
                }
                break;
            case confirmGoTo:
                _activeVehicle.guidedModeGotoLocation(_flightMap._gotoHereCoordinate)
                break;
            default:
                console.warn("Internal error: unknown confirmActionCode", confirmActionCode)
            }
        }

        function confirmAction(actionCode) {
            confirmActionCode = actionCode
            switch (confirmActionCode) {
            case confirmArm:
                _guidedModeBar.confirmText = "arm"
                break;
            case confirmDisarm:
                _guidedModeBar.confirmText = "disarm"
                break;
            case confirmEmergencyStop:
                _guidedModeBar.confirmText = "emergency stop"
                break;
            case confirmTakeoff:
                altitudeSlider.visible = true
                altitudeSlider.initialValue = 10
                _guidedModeBar.confirmText = "takeoff"
                break;
            case confirmLand:
                _guidedModeBar.confirmText = "land"
                break;
            case confirmChangeAlt:
                altitudeSlider.visible = true
                altitudeSlider.initialValue = _activeVehicle.altitudeAMSL.value
                _guidedModeBar.confirmText = "altitude change"
                break;
            case confirmGoTo:
                _guidedModeBar.confirmText = "move"
                break;
            }
            guidedModeButtons.visible = false
            guidedModeConfirm.visible = true
        }

        Row {
            id:                 guidedModeButtons
            anchors.margins:    _margins
            anchors.top:        parent.top
            anchors.left:       parent.left
            spacing:            _margins

            QGCButton {
                text:       _activeVehicle.armed ? (_activeVehicle.flying ? "Emergency Stop" : "Disarm") : "Arm"
                onClicked:  _guidedModeBar.confirmAction(_activeVehicle.armed ? (_activeVehicle.flying ? _guidedModeBar.confirmEmergencyStop : _guidedModeBar.confirmDisarm) : _guidedModeBar.confirmArm)
            }

            QGCButton {
                text:       "RTL"
                visible:    _activeVehicle.guidedModeSupported && _activeVehicle.flying
                onClicked:  _guidedModeBar.confirmAction(_guidedModeBar.confirmHome)
            }

            QGCButton {
                text:        _activeVehicle.flying ? "Land" : "Takeoff"
                visible:    _activeVehicle.guidedModeSupported && _activeVehicle.armed
                onClicked:  _guidedModeBar.confirmAction(_activeVehicle.flying ? _guidedModeBar.confirmLand : _guidedModeBar.confirmTakeoff)
            }

            QGCButton {
                text:       "Pause"
                visible:    _activeVehicle.pauseVehicleSupported && _activeVehicle.flying
                onClicked:  _activeVehicle.pauseVehicle()
            }

            QGCButton {
                text:       "Change Altitude"
                visible:    _activeVehicle.guidedModeSupported && _activeVehicle.armed
                onClicked:  _guidedModeBar.confirmAction(_guidedModeBar.confirmChangeAlt)
            }
        }

        Row {
            id:                 guidedModeConfirm
            anchors.margins:    _margins
            anchors.top:        parent.top
            anchors.left:       parent.left
            spacing:            _margins
            visible:            false

            QGCLabel {
                text: "Confirm " + _guidedModeBar.confirmText + " :"
                anchors.verticalCenter: parent.verticalCenter
            }

            QGCButton {
                text: "Yes"
                onClicked: {
                    guidedModeConfirm.visible = false
                    guidedModeButtons.visible = true
                    _guidedModeBar.actionConfirmed()
                    altitudeSlider.visible = false
                }
            }

            QGCButton {
                text: "No"
                onClicked: {
                    guidedModeConfirm.visible = false
                    guidedModeButtons.visible = true
                    altitudeSlider.visible = false
                    _flightMap._gotoHereCoordinate = QtPositioning.coordinate()
                }
            }
        }
    } // Column - Vertical tool buttons

    //-- Altitude slider
    Rectangle {
        id:                 altitudeSlider
        anchors.margins:    _margins
        anchors.right:      parent.right
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        color:              qgcPal.window
        width:              ScreenTools.defaultFontPixelWidth * 10
        opacity:            0.8
        visible:            false

        property alias initialValue:    altSlider.value

        /// Returns NaN for bad value
        function getValue() {
            return parseFloat(altField.text)
        }


        Column {
            id:                 headerColumn
            anchors.margins:    _margins
            anchors.top:        parent.top
            anchors.left:       parent.left
            anchors.right:      parent.right

            QGCLabel {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Altitude"
            }

            QGCLabel {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "meters (rel)"
            }

            QGCTextField {
                id:             altField
                anchors.left:   parent.left
                anchors.right:  parent.right
                text:           altSlider.value.toFixed(1)
            }
        }

        Slider {
            id:                 altSlider
            anchors.margins:    _margins
            anchors.top:        headerColumn.bottom
            anchors.bottom:     parent.bottom
            anchors.left:       parent.left
            anchors.right:      parent.right
            orientation:        Qt.Vertical
            minimumValue:       0
            maximumValue:       100
            value:              30
            //anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}

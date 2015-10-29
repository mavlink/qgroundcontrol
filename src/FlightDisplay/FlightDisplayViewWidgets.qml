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

    readonly property string _InstrumentVisibleKey: "IsInstrumentPanelVisible"

    property bool _isInstrumentVisible: QGroundControl.loadBoolGlobalSetting(_InstrumentVisibleKey, true)

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
        anchors.margins:        ScreenTools.defaultFontPixelHeight
        anchors.right:          parent.right
        anchors.verticalCenter: parent.verticalCenter
        visible:                _isInstrumentVisible
        size:                   mainWindow.width * 0.15
        active:                 _activeVehicle != null
        heading:                _heading
        rollAngle:              _roll
        pitchAngle:             _pitch
        altitude:               _altitudeWGS84
        groundSpeed:            _groundSpeed
        airSpeed:               _airSpeed
        isSatellite:            _mainIsMap ? _flightMap ? _flightMap.isSatelliteMap : true : true
        z:                      QGroundControl.zOrderWidgets
        onClicked: {
            _isInstrumentVisible = false
            QGroundControl.saveBoolGlobalSetting(_InstrumentVisibleKey, false)
        }
    }

    //-- Show (Hidden) Instrument Panel
    Rectangle {
        id:                     openButton
        anchors.right:          parent.right
        anchors.bottom:         parent.bottom
        anchors.margins:        ScreenTools.defaultFontPixelHeight
        height:                 ScreenTools.defaultFontPixelSize * 2
        width:                  ScreenTools.defaultFontPixelSize * 2
        radius:                 ScreenTools.defaultFontPixelSize / 3
        visible:                !_isInstrumentVisible
        color:                  _isBackgroundDark ? Qt.rgba(1,1,1,0.5) : Qt.rgba(0,0,0,0.5)
        Image {
            width:              parent.width  * 0.75
            height:             parent.height * 0.75
            source:             "/qmlimages/buttonLeft.svg"
            mipmap:             true
            fillMode:           Image.PreserveAspectFit
            anchors.verticalCenter:     parent.verticalCenter
            anchors.horizontalCenter:   parent.horizontalCenter
        }
        MouseArea {
            anchors.fill: parent
            onClicked: {
                _isInstrumentVisible = true
                QGroundControl.saveBoolGlobalSetting(_InstrumentVisibleKey, true)
            }
        }
    }

    //-- Vertical Tool Buttons
    Column {
        id:                         toolColumn
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
                            _flightMap.latitude  = activeVehicle.latitude
                            _flightMap.longitude = activeVehicle.longitude
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

}

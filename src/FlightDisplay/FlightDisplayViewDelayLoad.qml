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
            z:                  QGroundControl.zOrderWidgets
        }

        QGCAttitudeHUD {
            id:                 attitudeHUD
            rollAngle:          _roll
            pitchAngle:         _pitch
            width:              ScreenTools.defaultFontPixelSize * (30)
            height:             ScreenTools.defaultFontPixelSize * (30)
            active:             multiVehicleManager.activeVehicleAvailable
            z:                  QGroundControl.zOrderWidgets
        }
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

    //-- Instrument Pannel
    QGCInstrumentWidget {
        anchors.margins:    ScreenTools.defaultFontPixelHeight
        anchors.left:       parent.left
        anchors.bottom:     parent.bottom
        size:               ScreenTools.defaultFontPixelSize * (9)
        heading:            _heading
        rollAngle:          _roll
        pitchAngle:         _pitch
        z:                  QGroundControl.zOrderWidgets
    }

    //-- Map Center Control
    DropButton {
        id:                     centerMapDropButton
        anchors.rightMargin:    ScreenTools.defaultFontPixelHeight
        anchors.right:          mapTypeButton.left
        anchors.top:            mapTypeButton.top
        dropDirection:          dropDown
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
                    checked:            flightMap._followVehicle
                    anchors.baseline:   centerMapButton.baseline

                    onClicked: {
                        centerMapDropButton.hideDropDown()
                        flightMap._followVehicle = !flightMap._followVehicle
                    }
                }

                QGCButton {
                    id:         centerMapButton
                    text:       "Center map on Vehicle"
                    enabled:    _activeVehicle && !followVehicleCheckBox.checked

                    property var activeVehicle: multiVehicleManager.activeVehicle

                    onClicked: {
                        centerMapDropButton.hideDropDown()
                        flightMap.latitude = activeVehicle.latitude
                        flightMap.longitude = activeVehicle.longitude
                    }
                }
            }
        }
    }

    //-- Map Type Control
    DropButton {
        id:                     mapTypeButton
        anchors.topMargin:      topMargin
        anchors.rightMargin:    ScreenTools.defaultFontPixelHeight
        anchors.top:            parent.top
        anchors.right:          parent.right
        dropDirection:          dropDown
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
                        checked:    flightMap.mapType == text
                        text:       modelData

                        onClicked: {
                            flightMap.mapType = text
                            mapTypeButton.hideDropDown()
                        }
                    }
                }
            }
        }
    }

    //-- Temporary Options Button
    QGCButton {
        id:         optionsButton
        x:          _flightMap.mapWidgets.x
        y:          _flightMap.mapWidgets.y - height - (ScreenTools.defaultFontPixelHeight / 2)
        z:          QGroundControl.zOrderWidgets
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

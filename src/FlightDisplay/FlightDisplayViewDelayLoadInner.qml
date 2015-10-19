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

/// This component is used to delay load the controls which are children of the inner FlightMap
/// control of FlightDisplayView.
// Vehicle GPS lock display
Item {
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
                z:                      flightMap.zOrderMapItems - 2
            }
        }
    }

    QGCCompassWidget {
        anchors.leftMargin: ScreenTools.defaultFontPixelHeight
        anchors.topMargin:  topMargin
        anchors.left:       parent.left
        anchors.top:        parent.top
        size:               ScreenTools.defaultFontPixelSize * (13.3)
        heading:            _heading
        active:             multiVehicleManager.activeVehicleAvailable
        z:                  flightMap.zOrderWidgets
    }

    QGCAttitudeWidget {
        anchors.margins:    ScreenTools.defaultFontPixelHeight
        anchors.left:       parent.left
        anchors.bottom:     parent.bottom
        size:               ScreenTools.defaultFontPixelSize * (13.3)
        rollAngle:          _roll
        pitchAngle:         _pitch
        active:             multiVehicleManager.activeVehicleAvailable
        z:                  flightMap.zOrderWidgets
    }

    DropButton {
        id:                     centerMapDropButton
        anchors.rightMargin:    ScreenTools.defaultFontPixelHeight
        anchors.right:          mapTypeButton.left
        anchors.top:            mapTypeButton.top
        dropDirection:          dropDown
        buttonImage:            "/qmlimages/MapCenter.svg"
        viewportMargins:        ScreenTools.defaultFontPixelWidth / 2
        exclusiveGroup:         _dropButtonsExclusiveGroup
        z:                      flightMap.zOrderWidgets

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
        z:                      flightMap.zOrderWidgets

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
}

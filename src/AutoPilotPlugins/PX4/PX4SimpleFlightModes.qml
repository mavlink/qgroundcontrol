/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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
import QtQuick.Controls         1.2

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.Controllers   1.0
import QGroundControl.ScreenTools   1.0

Item {
    id: root

    // The following properties must be pushed in from the Loader
    //property var qgcView      - QGCView control
    //property var qgcViewPanel - QGCViewPanel control

    property real _margins: ScreenTools.defaultFontPixelHeight / 2

    QGCPalette { id: qgcPal; colorGroupEnabled: panel.enabled }

    PX4SimpleFlightModesController {
        id:         controller
        factPanel:  qgcViewPanel
    }

    QGCFlickable {
        anchors.fill:   parent
        clip:           true
        contentWidth:   contentColumn.width
        contentHeight:  contentColumn.height

        Column {
            id:         contentColumn
            spacing:    _margins

            QGCLabel {
                id:             flightModeLabel
                text:           "Flight Mode Settings"
                font.weight:    Font.DemiBold
            }

            Item {
                height: modeChannelCombo.height
                width:  modeChannelCombo.x + modeChannelCombo.width

                QGCLabel {
                    id:                 modeChannelLabel
                    anchors.baseline:   modeChannelCombo.baseline
                    text:               "Flight mode channel:"
                }

                FactComboBox {
                    id:                 modeChannelCombo
                    anchors.leftMargin: _margins
                    anchors.left:       modeChannelLabel.right
                    width:              ScreenTools.defaultFontPixelWidth * 15
                    fact:               controller.getParameterFact(-1, "RC_MAP_FLTMODE")
                    indexModel:         false
                }
            }

            Rectangle {
                id:                 flightModeSettings
                width:              flightModeColumn.width + (_margins * 2)
                height:             flightModeColumn.height + ScreenTools.defaultFontPixelHeight
                color:              qgcPal.windowShade

                Column {
                    id:                 flightModeColumn
                    anchors.margins:    ScreenTools.defaultFontPixelWidth
                    anchors.left:       parent.left
                    anchors.top:        parent.top
                    spacing:            ScreenTools.defaultFontPixelHeight

                    Repeater {
                        model:  6

                        Row {
                            spacing: ScreenTools.defaultFontPixelWidth

                            property int index:         modelData + 1
                            property var pwmStrings:    [ "PWM 0 - 1230", "PWM 1231 - 1360", "PWM 1361 - 1490", "PWM 1491 - 1620", "PWM 1621 - 1749", "PWM 1750 +"]


                            QGCLabel {
                                anchors.baseline:   modeCombo.baseline
                                text:               "Flight Mode " + index + ":"
                                color:              controller.activeFlightMode == index ? "yellow" : qgcPal.text
                            }

                            FactComboBox {
                                id:         modeCombo
                                width:      ScreenTools.defaultFontPixelWidth * 20
                                fact:       controller.getParameterFact(-1, "COM_FLTMODE" + index)
                                indexModel: false
                            }

                            QGCLabel {
                                anchors.baseline:   modeCombo.baseline
                                text:               pwmStrings[modelData]
                            }
                        }
                    } // Repeater - Flight Modes
                } // Column - Flight Modes
            } // Rectangle - Flight Modes

            QGCButton {
                text:           "Use Advanced Flight Modes"
                onClicked: {
                    controller.getParameterFact(-1, "RC_MAP_MODE_SW").value = 5
                    controller.getParameterFact(-1, "RC_MAP_FLTMODE").value = 0
                }
            }
        } // Column
    } // QGCFlickable
} // QGCView

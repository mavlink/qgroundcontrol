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

QGCView {
    id:         rootQGCView
    viewPanel:  panel

    property real   _margins:                   ScreenTools.defaultFontPixelHeight
    property bool   _channel7OptionsAvailable:  controller.parameterExists(-1, "CH7_OPT")   // Not available in all firmware types
    property bool   _channel9OptionsAvailable:  controller.parameterExists(-1, "CH9_OPT")   // Not available in all firmware types
    property int    _channelOptionCount:         _channel7OptionsAvailable ? (_channel9OptionsAvailable ? 6 : 2) : 0
    property Fact   _nullFact
    property bool   _fltmodeChExists:           controller.parameterExists(-1, "FLTMODE_CH")
    property Fact   _fltmodeCh:                 _fltmodeChExists ? controller.getParameterFact(-1, "FLTMODE_CH") : _nullFact

    QGCPalette { id: qgcPal; colorGroupEnabled: panel.enabled }

    APMFlightModesComponentController {
        id:         controller
        factPanel:  panel
    }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        QGCFlickable {
            anchors.fill:       parent
            clip:               true
            flickableDirection: Flickable.VerticalFlick
            contentHeight:      flightModeSettings.y + flightModeSettings.height

            QGCLabel {
                id:             flightModeLabel
                text:           "Flight Mode Settings" + (_fltmodeChExists ? "" : " (Channel 5)")
                font.weight:    Font.DemiBold
            }

            Rectangle {
                id:                 flightModeSettings
                anchors.topMargin:  _margins
                anchors.top:        flightModeLabel.bottom
                width:              flightModeColumn.width + (_margins * 2)
                height:             flightModeColumn.height + ScreenTools.defaultFontPixelHeight
                color:              qgcPal.windowShade

                Column {
                    id:                 flightModeColumn
                    anchors.margins:    ScreenTools.defaultFontPixelWidth
                    anchors.left:       parent.left
                    anchors.top:        parent.top
                    spacing:            ScreenTools.defaultFontPixelHeight

                    Row {
                        spacing:    _margins
                        visible:    _fltmodeChExists

                        QGCLabel {
                            id:                 modeChannelLabel
                            anchors.baseline:   modeChannelCombo.baseline
                            text:               "Flight mode channel:"
                        }

                        QGCComboBox {
                            id:             modeChannelCombo
                            width:          ScreenTools.defaultFontPixelWidth * 15
                            model:          [ "Not assigned", "Channel 1", "Channel 2","Channel 3","Channel 4","Channel 5","Channel 6","Channel 7","Channel 8" ]
                            currentIndex:   _fltmodeCh.value
                            onActivated:    _fltmodeCh.value = index
                        }
                    }

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
                                width:      ScreenTools.defaultFontPixelWidth * 15
                                fact:       controller.getParameterFact(-1, "FLTMODE" + index)
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

            QGCLabel {
                id:                 channelOptionsLabel
                anchors.leftMargin: _margins
                anchors.top:        parent.top
                anchors.left:       flightModeSettings.right
                text:               "Channel Options"
                font.weight:        Font.DemiBold
                visible:            _channelOptionCount != 0
            }

            Rectangle {
                id:                 channelOptionsSettings
                anchors.topMargin:  _margins
                anchors.top:        channelOptionsLabel.bottom
                anchors.left:       channelOptionsLabel.left
                width:              channelOptColumn.width + (_margins * 2)
                height:             channelOptColumn.height + ScreenTools.defaultFontPixelHeight
                color:              qgcPal.windowShade
                visible:            _channelOptionCount != 0

                Column {
                    id:                 channelOptColumn
                    anchors.margins:    ScreenTools.defaultFontPixelWidth
                    anchors.left:       parent.left
                    anchors.top:        parent.top
                    spacing:            ScreenTools.defaultFontPixelHeight

                    Repeater {
                        model: _channelOptionCount

                        Row {
                            spacing: ScreenTools.defaultFontPixelWidth

                            property int index: modelData + 7
                            property Fact nullFact: Fact { }

                            QGCLabel {
                                anchors.baseline:   optCombo.baseline
                                text:               "Channel option " + index + ":"
                                color:              controller.channelOptionEnabled[modelData] ? "yellow" : qgcPal.text
                            }

                            FactComboBox {
                                id:         optCombo
                                width:      ScreenTools.defaultFontPixelWidth * 15
                                fact:       controller.getParameterFact(-1, "CH" + index + "_OPT")
                                indexModel: false
                            }
                        }
                    } // Repeater -- Channel options
                } // Column - Channel options
            } // Rectangle - Channel options
        } // QGCFlickable
    } // QGCViewPanel
} // QGCView

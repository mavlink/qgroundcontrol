/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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

    readonly property real _flightModeComboWidth:   ScreenTools.defaultFontPixelWidth * 13
    readonly property real _channelComboWidth:      ScreenTools.defaultFontPixelWidth * 13

    QGCPalette { id: qgcPal; colorGroupEnabled: panel.enabled }

    PX4SimpleFlightModesController {
        id:         controller
        factPanel:  qgcViewPanel
    }

    QGCFlickable {
        anchors.fill:   parent
        clip:           true
        contentWidth:   column2.x + column2.width
        contentHeight:  Math.max(column1.height, column2.height)

        Column {
            id:         column1
            spacing:    _margins

            Row {
                id:         settingsRow
                spacing:    _margins

                Column {
                    id:     flightModeSettingsColumn
                    spacing: _margins

                    QGCLabel {
                        id:             flightModeLabel
                        text:           qsTr("Flight Mode Settings")
                        font.family:    ScreenTools.demiboldFontFamily
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

                            Row {
                                spacing: _margins

                                QGCLabel {
                                    id:                 modeChannelLabel
                                    anchors.baseline:   modeChannelCombo.baseline
                                    text:               qsTr("Mode channel:")
                                }

                                FactComboBox {
                                    id:         modeChannelCombo
                                    width:      _channelComboWidth
                                    fact:       controller.getParameterFact(-1, "RC_MAP_FLTMODE")
                                    indexModel: false
                                }
                            }

                            Repeater {
                                model:  6

                                Row {
                                    spacing: ScreenTools.defaultFontPixelWidth

                                    property int index:         modelData + 1

                                    QGCLabel {
                                        anchors.baseline:   modeCombo.baseline
                                        text:               qsTr("Flight Mode %1").arg(index)
                                        color:              controller.activeFlightMode == index ? "yellow" : qgcPal.text
                                    }

                                    FactComboBox {
                                        id:         modeCombo
                                        width:      _flightModeComboWidth
                                        fact:       controller.getParameterFact(-1, "COM_FLTMODE" + index)
                                        indexModel: false
                                    }
                                }
                            } // Repeater - Flight Modes
                        } // Column - Flight Modes
                    } // Rectangle - Flight Modes
                } // Column - Flight mode settings

                Column {
                    id:         column2
                    spacing:    _margins

                    QGCLabel {
                        text:           qsTr("Switch Settings")
                        font.family:    ScreenTools.demiboldFontFamily
                    }

                    Rectangle {
                        id:     switchSettingsRect
                        width:  switchSettingsColumn.width + (_margins * 2)
                        height: switchSettingsColumn.height + ScreenTools.defaultFontPixelHeight
                        color:  qgcPal.windowShade

                        Column {
                            id:                 switchSettingsColumn
                            anchors.margins:    ScreenTools.defaultFontPixelWidth
                            anchors.left:       parent.left
                            anchors.top:        parent.top
                            spacing:            ScreenTools.defaultFontPixelHeight

                            Row {
                                spacing: ScreenTools.defaultFontPixelWidth

                                property Fact fact: controller.getParameterFact(-1, "RC_MAP_RETURN_SW")

                                QGCLabel {
                                    anchors.baseline:   returnCombo.baseline
                                    text:               "Return switch:"
                                    color:              parent.fact.value == 0 ? qgcPal.text : (controller.rcChannelValues[parent.fact.value - 1] >= 1500 ? "yellow" : qgcPal.text)
                                }

                                FactComboBox {
                                    id:         returnCombo
                                    width:      _channelComboWidth
                                    fact:       parent.fact
                                    indexModel: false
                                }
                            }

                            Row {
                                spacing: ScreenTools.defaultFontPixelWidth

                                property Fact fact: controller.getParameterFact(-1, "RC_MAP_KILL_SW")

                                QGCLabel {
                                    anchors.baseline:   killCombo.baseline
                                    text:               "Kill switch:"
                                    color:              parent.fact.value == 0 ? qgcPal.text : (controller.rcChannelValues[parent.fact.value - 1] >= 1500 ? "yellow" : qgcPal.text)
                                }

                                FactComboBox {
                                    id:         killCombo
                                    width:      _channelComboWidth
                                    fact:       parent.fact
                                    indexModel: false
                                }
                            }

                            Row {
                                spacing: ScreenTools.defaultFontPixelWidth

                                property Fact fact: controller.getParameterFact(-1, "RC_MAP_OFFB_SW")

                                QGCLabel {
                                    anchors.baseline:   offboardCombo.baseline
                                    text:               "Offboard switch:"
                                    color:              parent.fact.value == 0 ? qgcPal.text : (controller.rcChannelValues[parent.fact.value - 1] >= 1500 ? "yellow" : qgcPal.text)
                                }

                                FactComboBox {
                                    id:         offboardCombo
                                    width:      _channelComboWidth
                                    fact:       parent.fact
                                    indexModel: false
                                }
                            }
                        } // Column
                    } // Rectangle

                    RCChannelMonitor {
                        width: switchSettingsRect.width
                    }
                } // Column - Switch settings
            } // Row - Settings

            QGCButton {
                text: "Use Multi Channel Mode Selection"
                onClicked: {
                    controller.getParameterFact(-1, "RC_MAP_MODE_SW").value = 5
                    controller.getParameterFact(-1, "RC_MAP_FLTMODE").value = 0
                }
            }
        } // Column
    } // QGCFlickable
} // QGCView

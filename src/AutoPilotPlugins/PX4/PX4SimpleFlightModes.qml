/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.2

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

                        ColumnLayout {
                            id:                 flightModeColumn
                            anchors.margins:    ScreenTools.defaultFontPixelWidth
                            anchors.left:       parent.left
                            anchors.top:        parent.top
                            spacing:            ScreenTools.defaultFontPixelHeight

                            RowLayout {
                                Layout.fillWidth:   true
                                spacing:            _margins

                                QGCLabel {
                                    Layout.fillWidth:   true
                                    text:               qsTr("Mode channel:")
                                }

                                FactComboBox {
                                    Layout.preferredWidth:  _channelComboWidth
                                    fact:                   controller.getParameterFact(-1, "RC_MAP_FLTMODE")
                                    indexModel:             false
                                }
                            }

                            Repeater {
                                model:  6

                                RowLayout {
                                    Layout.fillWidth:   true
                                    spacing:            ScreenTools.defaultFontPixelWidth

                                    property int index:         modelData + 1

                                    QGCLabel {
                                        Layout.fillWidth:   true
                                        text:               qsTr("Flight Mode %1").arg(index)
                                        color:              controller.activeFlightMode == index ? "yellow" : qgcPal.text
                                    }

                                    FactComboBox {
                                        Layout.preferredWidth:  _channelComboWidth
                                        fact:                   controller.getParameterFact(-1, "COM_FLTMODE" + index)
                                        indexModel:             false
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
                        width:  switchSettingsGrid.width + (_margins * 2)
                        height: switchSettingsGrid.height + ScreenTools.defaultFontPixelHeight
                        color:  qgcPal.windowShade

                        GridLayout {
                            id:                 switchSettingsGrid
                            anchors.margins:    ScreenTools.defaultFontPixelWidth
                            anchors.left:       parent.left
                            anchors.top:        parent.top
                            columns:            2
                            columnSpacing:      ScreenTools.defaultFontPixelWidth

                            Repeater {
                                model: [ "RC_MAP_ACRO_SW", "RC_MAP_ARM_SW", "RC_MAP_GEAR_SW", "RC_MAP_KILL_SW", "RC_MAP_LOITER_SW", "RC_MAP_OFFB_SW", "RC_MAP_POSCTL_SW", "RC_MAP_RATT_SW", "RC_MAP_RETURN_SW", "RC_MAP_STAB_SW" ]

                                RowLayout {
                                    spacing:            ScreenTools.defaultFontPixelWidth
                                    Layout.fillWidth:   true

                                    property Fact fact: controller.getParameterFact(-1, modelData)

                                    QGCLabel {
                                        text:               fact.shortDescription
                                        Layout.fillWidth:   true
                                    }

                                    FactComboBox {
                                        Layout.preferredWidth:  _channelComboWidth
                                        fact:                   parent.fact
                                        indexModel:             false
                                    }
                                }
                            }

                            Repeater {
                                model: [ "RC_MAP_FLAPS", "RC_MAP_MAN_SW" ]

                                RowLayout {
                                    spacing:            ScreenTools.defaultFontPixelWidth
                                    visible:            controller.vehicle.fixedWing
                                    Layout.fillWidth:   true

                                    property Fact fact: controller.getParameterFact(-1, modelData)

                                    QGCLabel {
                                        text:               fact.shortDescription
                                        Layout.fillWidth:   true
                                    }

                                    FactComboBox {
                                        Layout.preferredWidth:  _channelComboWidth
                                        fact:                   parent.fact
                                        indexModel:             false
                                    }
                                }
                            }

                            Repeater {
                                model: [ "RC_MAP_TRANS_SW" ]

                                RowLayout {
                                    spacing:            ScreenTools.defaultFontPixelWidth
                                    Layout.fillWidth:   true
                                    visible:            controller.vehicle.vtol

                                    property Fact fact: controller.getParameterFact(-1, modelData)

                                    QGCLabel {
                                        text:               fact.shortDescription
                                        Layout.fillWidth:   true
                                    }

                                    FactComboBox {
                                        Layout.preferredWidth:  _channelComboWidth
                                        fact:                   parent.fact
                                        indexModel:             false
                                    }
                                }
                            }


                            Row {
                                spacing: ScreenTools.defaultFontPixelWidth

                                property Fact fact: controller.getParameterFact(-1, "RC_MAP_TRANS_SW", false)
                                visible: (controller.vehicle.vtol && controller.parameterExists(-1, "RC_MAP_TRANS_SW"))

                                QGCLabel {
                                    anchors.baseline:   vtolCombo.baseline
                                    text:               qsTr("VTOL mode switch:")
                                    color:              parent.fact.value === 0 ? qgcPal.text : (controller.rcChannelValues[parent.fact.value - 1] >= 1500 ? "yellow" : qgcPal.text)
                                }

                                FactComboBox {
                                    id:         vtolCombo
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

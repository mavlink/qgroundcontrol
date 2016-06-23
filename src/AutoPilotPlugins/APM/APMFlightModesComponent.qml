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

QGCView {
    id:         rootQGCView
    viewPanel:  panel

    readonly property string _modeChannelParam: controller.modeChannelParam
    readonly property string _modeParamPrefix:  controller.modeParamPrefix

    property real   _margins:                   ScreenTools.defaultFontPixelHeight
    property bool   _channel7OptionsAvailable:  controller.parameterExists(-1, "CH7_OPT")   // Not available in all firmware types
    property bool   _channel9OptionsAvailable:  controller.parameterExists(-1, "CH9_OPT")   // Not available in all firmware types
    property int    _channelOptionCount:         _channel7OptionsAvailable ? (_channel9OptionsAvailable ? 6 : 2) : 0
    property Fact   _nullFact
    property bool   _fltmodeChExists:           controller.parameterExists(-1, _modeChannelParam)
    property Fact   _fltmodeCh:                 _fltmodeChExists ? controller.getParameterFact(-1, _modeChannelParam) : _nullFact

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
            contentHeight:      flowLayout.height
            contentWidth:       flowLayout.width

            Flow {
                id:         flowLayout
                width:      panel.width // parent.width doesn't work here for some reason!
                spacing:     _margins

                Column {
                    spacing: _margins

                    QGCLabel {
                        id:             flightModeLabel
                        text:           qsTr("Flight Mode Settings") + (_fltmodeChExists ? "" : qsTr(" (Channel 5)"))
                        font.family:    ScreenTools.demiboldFontFamily
                    }

                    Rectangle {
                        id:     flightModeSettings
                        width:  flightModeColumn.width + (_margins * 2)
                        height: flightModeColumn.height + ScreenTools.defaultFontPixelHeight
                        color:  qgcPal.windowShade

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
                                    text:               qsTr("Flight mode channel:")
                                }

                                QGCComboBox {
                                    id:             modeChannelCombo
                                    width:          ScreenTools.defaultFontPixelWidth * 15
                                    model:          [ qsTr("Not assigned"), qsTr("Channel 1"), qsTr("Channel 2"),
                                        qsTr("Channel 3"),    qsTr("Channel 4"), qsTr("Channel 5"),
                                        qsTr("Channel 6"),    qsTr("Channel 7"), qsTr("Channel 8") ]

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
                                        text:               qsTr("Flight Mode ") + index + ":"
                                        color:              controller.activeFlightMode == index ? "yellow" : qgcPal.text
                                    }

                                    FactComboBox {
                                        id:         modeCombo
                                        width:      ScreenTools.defaultFontPixelWidth * 15
                                        fact:       controller.getParameterFact(-1, _modeParamPrefix + index)
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
                } // Column - Flight Modes

                Column {
                    spacing:    _margins
                    visible:    _channelOptionCount != 0

                    QGCLabel {
                        id:                 channelOptionsLabel
                        text:               qsTr("Channel Options")
                        font.family:        ScreenTools.demiboldFontFamily
                    }

                    Rectangle {
                        id:     channelOptionsSettings
                        width:  channelOptColumn.width + (_margins * 2)
                        height: channelOptColumn.height + ScreenTools.defaultFontPixelHeight
                        color:  qgcPal.windowShade

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
                                        text:               qsTr("Channel option %1 :").arg(index)
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
                } // Column - Channel options
            } // Flow
        } // QGCFlickable
    } // QGCViewPanel
} // QGCView

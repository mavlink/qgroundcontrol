/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

    property real _margins:             ScreenTools.defaultFontPixelHeight / 2
    property var  _switchNameList:      [ "ACRO", "ARM", "GEAR", "KILL", "LOITER", "OFFB", "POSCTL", "RATT", "RETURN", "STAB" ]
    property var  _switchFactList:      [ ]
    property var  _switchTHFactList:    [ ]

    readonly property real _flightModeComboWidth:   ScreenTools.defaultFontPixelWidth * 13
    readonly property real _channelComboWidth:      ScreenTools.defaultFontPixelWidth * 13

    Component.onCompleted: {
        if (controller.vehicle.fixedWing) {
            _switchNameList.push("MAN")
        }
        if (controller.vehicle.vtol) {
            _switchNameList.push("TRANS")
        }
        for (var i=0; i<_switchNameList.length; i++) {
            _switchFactList.push("RC_MAP_" + _switchNameList[i] + "_SW")
            _switchTHFactList.push("RC_" + _switchNameList[i] + "_TH")
        }
        if (controller.vehicle.fixedWing) {
            _switchFactList.push("RC_MAP_FLAPS")
            _switchTHFactList.push("")
        }
        switchRepeater.model = _switchFactList
    }

    PX4SimpleFlightModesController {
        id:         controller
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

                        GridLayout {
                            id:                 flightModeColumn
                            anchors.margins:    ScreenTools.defaultFontPixelWidth
                            anchors.left:       parent.left
                            anchors.top:        parent.top
                            rows:               7
                            rowSpacing:         ScreenTools.defaultFontPixelWidth / 2
                            columnSpacing:      rowSpacing
                            flow:               GridLayout.TopToBottom

                            QGCLabel {
                                Layout.fillWidth:   true
                                text:               qsTr("Mode Channel")
                            }

                            Repeater {
                                model:  6

                                QGCLabel {
                                    Layout.fillWidth:   true
                                    text:               qsTr("Flight Mode %1").arg(modelData + 1)
                                    color:              controller.activeFlightMode == index ? "yellow" : qgcPal.text
                                }
                            }

                            FactComboBox {
                                Layout.fillWidth:   true
                                fact:               controller.getParameterFact(-1, "RC_MAP_FLTMODE")
                                indexModel:         false
                                sizeToContents:     true
                            }

                            Repeater {
                                model:  6

                                FactComboBox {
                                    Layout.fillWidth:   true
                                    fact:               controller.getParameterFact(-1, "COM_FLTMODE" + (modelData + 1))
                                    indexModel:         false
                                    sizeToContents:     true
                                }
                            }
                        }
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
                                id: switchRepeater

                                RowLayout {
                                    spacing:            ScreenTools.defaultFontPixelWidth
                                    Layout.fillWidth:   true

                                    property string thFactName:     _switchTHFactList[index]
                                    property bool   thFactExists:   thFactName == ""
                                    property Fact   swFact:         controller.getParameterFact(-1, modelData)
                                    property Fact   thFact:         thFactExists ? controller.getParameterFact(-1, thFactName) : null
                                    property real   thValue:        thFactExists ? thFact.rawValue : 0.5
                                    property real   thPWM:          1000 + (1000 * thValue)
                                    property int    swChannel:      swFact.rawValue - 1
                                    property bool   swActive:       swChannel < 0 ?
                                                                        false :
                                                                        (thValue >= 0 ?
                                                                             (controller.rcChannelValues[swChannel] > thPWM) :
                                                                             (controller.rcChannelValues[swChannel] <= thPWM))
                                    QGCLabel {
                                        text:               swFact.shortDescription
                                        Layout.fillWidth:   true
                                        color:              swActive ? "yellow" : qgcPal.text
                                    }

                                    FactComboBox {
                                        Layout.preferredWidth:  _channelComboWidth
                                        fact:                   swFact
                                        indexModel:             false
                                    }
                                }
                            }
                        } // Column
                    } // Rectangle

                    RCChannelMonitor {
                        width:      switchSettingsRect.width
                        twoColumn:  true
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
        }
    }
}

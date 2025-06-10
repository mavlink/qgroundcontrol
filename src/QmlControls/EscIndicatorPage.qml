/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.MultiVehicleManager
import QGroundControl.ScreenTools
import QGroundControl.Palette
import QGroundControl.FactSystem
import QGroundControl.FactControls

ToolIndicatorPage {
    showExpand: false

    property var    activeVehicle:      QGroundControl.multiVehicleManager.activeVehicle
    property string na:                 qsTr("N/A", "No data to display")
    property string valueNA:            qsTr("--.--", "No data to display")
    property var    _escStatus:         activeVehicle ? activeVehicle.escStatus : null

    // ESC data from vehicle fact groups
    property bool   _escDataAvailable:  _escStatus ? _escStatus.telemetryAvailable : false
    property var    _escList:           _escDataAvailable ? [
        {
            id: 1,
            rpm: _escStatus.rpmFirst.rawValue,
            current: _escStatus.currentFirst.rawValue,
            voltage: _escStatus.voltageFirst.rawValue,
            healthy: _escStatus.rpmFirst.rawValue >= 0
        },
        {
            id: 2,
            rpm: _escStatus.rpmSecond.rawValue,
            current: _escStatus.currentSecond.rawValue,
            voltage: _escStatus.voltageSecond.rawValue,
            healthy: _escStatus.rpmSecond.rawValue >= 0
        },
        {
            id: 3,
            rpm: _escStatus.rpmThird.rawValue,
            current: _escStatus.currentThird.rawValue,
            voltage: _escStatus.voltageThird.rawValue,
            healthy: _escStatus.rpmThird.rawValue >= 0
        },
        {
            id: 4,
            rpm: _escStatus.rpmFourth.rawValue,
            current: _escStatus.currentFourth.rawValue,
            voltage: _escStatus.voltageFourth.rawValue,
            healthy: _escStatus.rpmFourth.rawValue >= 0
        }
    ] : []

    contentComponent: Component {
        ColumnLayout {
            spacing: ScreenTools.defaultFontPixelHeight / 2

            SettingsGroupLayout {
                heading: qsTr("ESC Status Overview")
                visible: activeVehicle

                LabelledLabel {
                    label:      qsTr("ESC Count")
                    labelText:  _escDataAvailable ? _escList.length.toString() : na
                }

                LabelledLabel {
                    label:      qsTr("Overall Status")
                    labelText:  {
                        if (!_escDataAvailable) return na
                        var healthyCount = 0
                        for (var i = 0; i < _escList.length; i++) {
                            if (_escList[i].healthy) healthyCount++
                        }
                        return healthyCount + "/" + _escList.length + " " + qsTr("Healthy")
                    }
                }

                LabelledLabel {
                    label:      qsTr("Max Current")
                    labelText:  {
                        if (!_escDataAvailable) return valueNA
                        var maxCurrent = 0
                        for (var i = 0; i < _escList.length; i++) {
                            if (_escList[i].current > maxCurrent) maxCurrent = _escList[i].current
                        }
                        return maxCurrent.toFixed(1) + "A"
                    }
                }

                LabelledLabel {
                    label:      qsTr("Max RPM")
                    labelText:  {
                        if (!_escDataAvailable) return valueNA
                        var maxRpm = 0
                        for (var i = 0; i < _escList.length; i++) {
                            if (_escList[i].rpm > maxRpm) maxRpm = _escList[i].rpm
                        }
                        return maxRpm.toString()
                    }
                }
            }

            SettingsGroupLayout {
                heading: qsTr("Individual ESC Status")
                visible: _escDataAvailable && _escList.length > 0

                Repeater {
                    model: _escDataAvailable ? _escList : []

                    SettingsGroupLayout {
                        heading: qsTr("ESC %1").arg(modelData.id)
                        contentSpacing: 0
                        showDividers: false

                        Rectangle {
                            Layout.fillWidth: true
                            height: ScreenTools.defaultFontPixelHeight * 0.1
                            color: modelData.healthy ? qgcPal.colorGreen : qgcPal.colorRed
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: ScreenTools.defaultFontPixelWidth

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                LabelledLabel {
                                    label: qsTr("RPM")
                                    labelText: modelData.rpm.toString()
                                }

                                LabelledLabel {
                                    label: qsTr("Voltage")
                                    labelText: modelData.voltage.toFixed(1) + "V"
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                LabelledLabel {
                                    label: qsTr("Current")
                                    labelText: modelData.current.toFixed(1) + "A"
                                }

                                LabelledLabel {
                                    label: qsTr("Health")
                                    labelText: modelData.healthy ? qsTr("OK") : qsTr("ERROR")
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                LabelledLabel {
                                    label: qsTr("Index")
                                    labelText: _escStatus ? _escStatus.index.valueString : na
                                }

                                QGCLabel {
                                    text: " "  // Spacer
                                }
                            }
                        }
                    }
                }
            }

            SettingsGroupLayout {
                heading: qsTr("ESC Information")
                visible: !_escDataAvailable

                QGCLabel {
                    text: qsTr("No ESC data available. ESC status information will be displayed here when the vehicle sends ESC_INFO and ESC_STATUS MAVLink messages.")
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }
    }
}
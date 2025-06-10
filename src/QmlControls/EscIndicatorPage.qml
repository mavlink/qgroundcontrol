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
    property int    _motorCount:        _escStatus ? _escStatus.count.rawValue : 0
    property int    _infoBitmask:       _escStatus ? _escStatus.info.rawValue : 0

    function _isMotorOnline(motorIndex) {
        // Check if the motor is online using the info bitmask
        return (_infoBitmask & (1 << motorIndex)) !== 0
    }

    function _getMotorData(motorIndex) {
        // Get motor data based on index (0-7)
        var motorData = {
            id: motorIndex + 1,
            rpm: 0,
            current: 0,
            voltage: 0,
            temperature: 0,
            errorCount: 0,
            failureFlags: 0,
            online: _isMotorOnline(motorIndex),
            healthy: false
        }

        switch (motorIndex) {
        case 0:
            motorData.rpm = _escStatus.rpmFirst.rawValue;
            motorData.current = _escStatus.currentFirst.rawValue;
            motorData.voltage = _escStatus.voltageFirst.rawValue;
            motorData.temperature = _escStatus.temperatureFirst.rawValue;
            motorData.errorCount = _escStatus.errorCountFirst.rawValue;
            motorData.failureFlags = _escStatus.failureFlagsFirst.rawValue;
            break;
        case 1:
            motorData.rpm = _escStatus.rpmSecond.rawValue;
            motorData.current = _escStatus.currentSecond.rawValue;
            motorData.voltage = _escStatus.voltageSecond.rawValue;
            motorData.temperature = _escStatus.temperatureSecond.rawValue;
            motorData.errorCount = _escStatus.errorCountSecond.rawValue;
            motorData.failureFlags = _escStatus.failureFlagsSecond.rawValue;
            break;
        case 2:
            motorData.rpm = _escStatus.rpmThird.rawValue;
            motorData.current = _escStatus.currentThird.rawValue;
            motorData.voltage = _escStatus.voltageThird.rawValue;
            motorData.temperature = _escStatus.temperatureThird.rawValue;
            motorData.errorCount = _escStatus.errorCountThird.rawValue;
            motorData.failureFlags = _escStatus.failureFlagsThird.rawValue;
            break;
        case 3:
            motorData.rpm = _escStatus.rpmFourth.rawValue;
            motorData.current = _escStatus.currentFourth.rawValue;
            motorData.voltage = _escStatus.voltageFourth.rawValue;
            motorData.temperature = _escStatus.temperatureFourth.rawValue;
            motorData.errorCount = _escStatus.errorCountFourth.rawValue;
            motorData.failureFlags = _escStatus.failureFlagsFourth.rawValue;
            break;
        case 4:
            motorData.rpm = _escStatus.rpmFifth.rawValue;
            motorData.current = _escStatus.currentFifth.rawValue;
            motorData.voltage = _escStatus.voltageFifth.rawValue;
            motorData.temperature = _escStatus.temperatureFifth.rawValue;
            motorData.errorCount = _escStatus.errorCountFifth.rawValue;
            motorData.failureFlags = _escStatus.failureFlagsFifth.rawValue;
            break;
        case 5:
            motorData.rpm = _escStatus.rpmSixth.rawValue;
            motorData.current = _escStatus.currentSixth.rawValue;
            motorData.voltage = _escStatus.voltageSixth.rawValue;
            motorData.temperature = _escStatus.temperatureSixth.rawValue;
            motorData.errorCount = _escStatus.errorCountSixth.rawValue;
            motorData.failureFlags = _escStatus.failureFlagsSixth.rawValue;
            break;
        case 6:
            motorData.rpm = _escStatus.rpmSeventh.rawValue;
            motorData.current = _escStatus.currentSeventh.rawValue;
            motorData.voltage = _escStatus.voltageSeventh.rawValue;
            motorData.temperature = _escStatus.temperatureSeventh.rawValue;
            motorData.errorCount = _escStatus.errorCountSeventh.rawValue;
            motorData.failureFlags = _escStatus.failureFlagsSeventh.rawValue;
            break;
        case 7:
            motorData.rpm = _escStatus.rpmEighth.rawValue;
            motorData.current = _escStatus.currentEighth.rawValue;
            motorData.voltage = _escStatus.voltageEighth.rawValue;
            motorData.temperature = _escStatus.temperatureEighth.rawValue;
            motorData.errorCount = _escStatus.errorCountEighth.rawValue;
            motorData.failureFlags = _escStatus.failureFlagsEighth.rawValue;
            break;
        }

        // Set healthy status: motor is healthy if it's online and has no failure flags
        motorData.healthy = motorData.online && motorData.failureFlags === 0

        return motorData
    }

    property var    _allMotors:         {
        var motors = []
        if (_escDataAvailable) {
            for (var i = 0; i < Math.min(_motorCount, 8); i++) {
                var motor = _getMotorData(i)
                if (motor.online) {
                    motors.push(motor)
                }
            }
        }
        return motors
    }

    contentComponent: Component {
        ColumnLayout {
            spacing: ScreenTools.defaultFontPixelHeight / 2

            SettingsGroupLayout {
                heading: qsTr("ESC Status Overview")
                visible: activeVehicle

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    rowSpacing: ScreenTools.defaultFontPixelHeight * 0.25
                    columnSpacing: ScreenTools.defaultFontPixelWidth * 2

                    LabelledLabel {
                        Layout.fillWidth: true
                        label:      qsTr("Healthy Motors")
                        labelText:  {
                            if (!_escDataAvailable) return na
                            var healthyCount = 0
                            for (var i = 0; i < _allMotors.length; i++) {
                                if (_allMotors[i].healthy) healthyCount++
                            }
                            return healthyCount + "/" + _motorCount
                        }
                    }

                    LabelledLabel {
                        Layout.fillWidth: true
                        label:      qsTr("Max Temperature")
                        labelText:  {
                            if (!_escDataAvailable || _allMotors.length === 0) return valueNA
                            var maxTemp = 0
                            var hasTemperature = false
                            for (var i = 0; i < _allMotors.length; i++) {
                                if (_allMotors[i].temperature > 0) {
                                    hasTemperature = true
                                    if (_allMotors[i].temperature > maxTemp) maxTemp = _allMotors[i].temperature
                                }
                            }
                            return hasTemperature ? maxTemp.toFixed(1) + "°C" : na
                        }
                    }

                    LabelledLabel {
                        Layout.fillWidth: true
                        label:      qsTr("Total Errors")
                        labelText:  {
                            if (!_escDataAvailable || _allMotors.length === 0) return na
                            var totalErrors = 0
                            for (var i = 0; i < _allMotors.length; i++) {
                                totalErrors += _allMotors[i].errorCount
                            }
                            return totalErrors.toString()
                        }
                    }

                    LabelledLabel {
                        Layout.fillWidth: true
                        label:      qsTr("Max Current")
                        labelText:  {
                            if (!_escDataAvailable || _allMotors.length === 0) return valueNA
                            var maxCurrent = 0
                            for (var i = 0; i < _allMotors.length; i++) {
                                if (_allMotors[i].current > maxCurrent) maxCurrent = _allMotors[i].current
                            }
                            return maxCurrent.toFixed(1) + "A"
                        }
                    }
                }
            }

            SettingsGroupLayout {
                heading: qsTr("Individual Motor Status")
                visible: _escDataAvailable && _allMotors.length > 0

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2  // 2 columns, each containing up to 4 motors
                    rowSpacing: ScreenTools.defaultFontPixelHeight * 0.5
                    columnSpacing: ScreenTools.defaultFontPixelWidth * 2

                    // Column 1 - Motors 1-4
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: ScreenTools.defaultFontPixelHeight * 0.25

                        Repeater {
                            model: Math.min(4, _allMotors.length)

                            ColumnLayout {
                                spacing: ScreenTools.defaultFontPixelHeight * 0.1
                                Layout.bottomMargin: index < Math.min(4, _allMotors.length) - 1 ? ScreenTools.defaultFontPixelHeight * 0.5 : 0

                                QGCLabel {
                                    text: qsTr("Motor %1").arg(_allMotors[index].id)
                                    color: qgcPal.text
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: ScreenTools.defaultFontPixelHeight * 0.15
                                    color: _allMotors[index].healthy ? qgcPal.colorGreen : qgcPal.colorRed
                                    radius: 2
                                }

                                GridLayout {
                                    Layout.fillWidth: true
                                    columns: 2
                                    rowSpacing: 2
                                    columnSpacing: ScreenTools.defaultFontPixelWidth * 2

                                    LabelledLabel {
                                        label: qsTr("RPM")
                                        labelText: _allMotors[index].rpm.toString()
                                    }

                                    LabelledLabel {
                                        label: qsTr("Temp")
                                        labelText: _allMotors[index].temperature > 0 ? _allMotors[index].temperature.toFixed(1) + "°C" : na
                                    }

                                    LabelledLabel {
                                        label: qsTr("Voltage")
                                        labelText: _allMotors[index].voltage.toFixed(1) + "V"
                                    }

                                    LabelledLabel {
                                        label: qsTr("Current")
                                        labelText: _allMotors[index].current.toFixed(1) + "A"
                                    }

                                    LabelledLabel {
                                        // Layout.fillWidth: true
                                        label: qsTr("Errors")
                                        labelText: _allMotors[index].errorCount.toString()
                                    }

                                    // Empty cell
                                    Item { }

                                }
                            }
                        }
                    }

                    // Column 2 - Motors 5-8
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: ScreenTools.defaultFontPixelHeight * 0.25
                        visible: _allMotors.length > 4

                        Repeater {
                            model: Math.max(0, _allMotors.length - 4)

                            ColumnLayout {
                                spacing: ScreenTools.defaultFontPixelHeight * 0.1
                                Layout.bottomMargin: index < Math.max(0, _allMotors.length - 4) - 1 ? ScreenTools.defaultFontPixelHeight * 0.5 : 0

                                QGCLabel {
                                    text: qsTr("Motor %1").arg(_allMotors[index + 4].id)
                                    font.pointSize: ScreenTools.smallFontPointSize
                                    color: qgcPal.text
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: ScreenTools.defaultFontPixelHeight * 0.15
                                    color: _allMotors[index + 4].healthy ? qgcPal.colorGreen : qgcPal.colorRed
                                    radius: 2
                                }

                                GridLayout {
                                    Layout.fillWidth: true
                                    columns: 2
                                    rowSpacing: 2
                                    columnSpacing: ScreenTools.defaultFontPixelWidth

                                    LabelledLabel {
                                        Layout.fillWidth: true
                                        label: qsTr("RPM")
                                        labelText: _allMotors[index + 4].rpm.toString()
                                    }

                                    LabelledLabel {
                                        Layout.fillWidth: true
                                        label: qsTr("Current")
                                        labelText: _allMotors[index + 4].current.toFixed(1) + "A"
                                    }

                                    LabelledLabel {
                                        Layout.fillWidth: true
                                        label: qsTr("Voltage")
                                        labelText: _allMotors[index + 4].voltage.toFixed(1) + "V"
                                    }

                                    LabelledLabel {
                                        Layout.fillWidth: true
                                        label: qsTr("Temp")
                                        labelText: _allMotors[index + 4].temperature > 0 ? _allMotors[index + 4].temperature.toFixed(1) + "°C" : na
                                    }

                                    LabelledLabel {
                                        Layout.fillWidth: true
                                        label: qsTr("Errors")
                                        labelText: _allMotors[index + 4].errorCount.toString()
                                    }

                                    // Empty cell
                                    Item { Layout.fillWidth: true }
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

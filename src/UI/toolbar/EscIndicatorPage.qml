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
import QGroundControl.FactControls

ToolIndicatorPage {
    id:             control
    showExpand:     false

    property var    activeVehicle:  QGroundControl.multiVehicleManager.activeVehicle
    property string na:             qsTr("N/A", "No data to display")
    property string valueNA:        qsTr("--.--", "No data to display")

    property var    _escs:          activeVehicle ? activeVehicle.escs : null
    property int    _onlineBitmask: _escs ? _escs.get(0).info.rawValue : 0

    function _isMotorOnline(motorIndex) {
        return (_onlineBitmask & (1 << motorIndex)) !== 0
    }

    function _isMotorHealthy(motorIndex) {
        return _isMotorOnline(motorIndex) && _escs.get(motorIndex).failureFlags.rawValue === 0
    }

    contentComponent: Component {
        ColumnLayout {
            spacing: ScreenTools.defaultFontPixelHeight / 4

            SettingsGroupLayout {
                heading: qsTr("ESC Status Overview")

                GridLayout {
                    Layout.fillWidth:   true
                    columns:            1
                    rowSpacing:         ScreenTools.defaultFontPixelHeight * 0.25
                    columnSpacing:      ScreenTools.defaultFontPixelWidth * 2

                    LabelledLabel {
                        Layout.fillWidth:   true
                        label:              qsTr("Healthy Motors")
                        labelText:  {
                            let healthyCount = 0
                            for (let i = 0; i < _escs.count; i++) {
                                if (_isMotorHealthy(i)) healthyCount++
                            }
                            return healthyCount + "/" + _escs.count
                        }
                    }

                    LabelledLabel {
                        Layout.fillWidth: true
                        label:      qsTr("Total Errors")
                        labelText:  {
                            let totalErrors = 0
                            for (let i = 0; i < _escs.count; i++) {
                                totalErrors += _escs.get(i).errorCount.rawValue
                            }
                            return totalErrors.toString()
                        }
                    }
                }
            }

            Repeater  {
                model: _escs

                SettingsGroupLayout {
                    heading:            qsTr("Motor %1 %2").arg(object.id.rawValue + 1).arg( _isThisMotorHealthy ? "" : qsTr("- OFFLINE"))
                    headingPointSize:   ScreenTools.defaultFontPointSize * ScreenTools.smallFontPointRatio
                    outerBorderColor:   _isThisMotorHealthy ? QGroundControl.globalPalette.colorGreen : QGroundControl.globalPalette.colorRed

                    property bool _isThisMotorHealthy: _isMotorHealthy(index)

                    GridLayout {
                        columns:        2
                        columnSpacing:  ScreenTools.defaultFontPixelWidth * 2.5
                        flow:       GridLayout.LeftToRight

                        LabelledFactLabel {
                            label:      qsTr("RPM")
                            fact:       object.rpm
                        }

                        LabelledLabel {
                            label:      qsTr("Temp")
                            labelText:  (object.temperature !== 32767 ? object.temperature.valueString : na) + " " + object.temperature.units
                        }

                        LabelledFactLabel {
                            label:      qsTr("Voltage")
                            fact:       object.voltage
                        }

                        LabelledFactLabel {
                            label:      qsTr("Current")
                            fact:       object.current
                        }

                        LabelledFactLabel {
                            label:      qsTr("Errors")
                            fact:       object.errorCount
                        }
                    }
                }
            }
        }
    }
}

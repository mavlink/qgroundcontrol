/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
import MAVLink

BatteryIndicator {
    waitForParameters: true

    expandedPageComponent: Component {
        ColumnLayout {
            FactPanelController { id: controller }

            property Fact batt1Monitor: controller.getParameterFact(-1, "BATT_MONITOR")
            property string disabledString: qsTr("- disabled")

            SettingsGroupLayout {
                Layout.fillWidth:   true
                heading:            qsTr("Low Voltage Failsafe")
                visible:            batt1Monitor.rawValue !== 0

                LabelledFactComboBox {
                    label:              qsTr("Vehicle Action")
                    fact:               controller.getParameterFact(-1, "BATT_FS_LOW_ACT")
                    indexModel:         false
                }

                FactSlider {
                    Layout.fillWidth:   true
                    label:              qsTr("Voltage Trigger") + (value == 0 ? disabledString : "")
                    fact:               controller.getParameterFact(-1, "BATT_LOW_VOLT")
                    from:               0
                    to:                 100
                    majorTickStepSize:  5
                }

                FactSlider {
                    Layout.fillWidth:   true
                    label:              qsTr("mAh Trigger") + (value == 0 ? disabledString : "")
                    fact:               controller.getParameterFact(-1, "BATT_LOW_MAH")
                    from:               0
                    to:                 30000
                    majorTickStepSize:  1000
                }
            }

            SettingsGroupLayout {
                Layout.fillWidth:   true
                heading:            qsTr("Critical Voltage Failsafe")
                visible:            batt1Monitor.rawValue !== 0

                LabelledFactComboBox {
                    label:              qsTr("Vehicle Action")
                    fact:               controller.getParameterFact(-1, "BATT_FS_CRT_ACT")
                    indexModel:         false
                }

                FactSlider {
                    Layout.fillWidth:   true
                    label:              qsTr("Voltage Trigger") + (value == 0 ? disabledString : "")
                    fact:               controller.getParameterFact(-1, "BATT_CRT_VOLT")
                    from:               0
                    to:                 100
                    majorTickStepSize:  5
                }

                FactSlider {
                    Layout.fillWidth:   true
                    label:              qsTr("mAh Trigger") + (value == 0 ? disabledString : "")
                    fact:               controller.getParameterFact(-1, "BATT_CRT_MAH")
                    from:               0
                    to:                 30000
                    majorTickStepSize:  1000
                }
            }
        }
    }
}

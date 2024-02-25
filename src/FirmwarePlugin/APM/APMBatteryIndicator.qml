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
        SettingsGroupLayout {
            Layout.fillWidth:   true
            heading:            qsTr("Battery Failsafes")
            visible:            batt1Monitor.rawValue !== 0

            FactPanelController { id: controller }

            property Fact batt1Monitor: controller.getParameterFact(-1, "BATT_MONITOR")

            LabelledFactComboBox {
                label:              qsTr("Low Action")
                fact:               controller.getParameterFact(-1, "BATT_FS_LOW_ACT")
                indexModel:         false
            }

            LabelledFactTextField {
                Layout.fillWidth:   true
                label:              qsTr("Low Voltage Threshold")
                fact:               controller.getParameterFact(-1, "BATT_LOW_VOLT")
            }

            LabelledFactTextField {
                Layout.fillWidth:   true
                label:              qsTr("Low mAh Threshold")
                fact:               controller.getParameterFact(-1, "BATT_LOW_MAH")
            }

            LabelledFactComboBox {
                label:              qsTr("Critical Action")
                fact:               controller.getParameterFact(-1, "BATT_FS_LOW_ACT")
                indexModel:         false
            }

            LabelledFactTextField {
                Layout.fillWidth:   true
                label:              qsTr("Low Voltage Threshold")
                fact:               controller.getParameterFact(-1, "BATT_CRT_VOLT")
            }

            LabelledFactTextField {
                Layout.fillWidth:   true
                label:              qsTr("Low mAh Threshold")
                fact:               controller.getParameterFact(-1, "BATT_CRT_MAH")
            }
        }
    }
}

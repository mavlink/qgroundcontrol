import QtQuick 2.3
import QtQuick.Controls 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0

FactPanel {
    id:             panel
    anchors.fill:   parent
    color:          qgcPal.windowShadeDark

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }
    FactPanelController { id: controller; factPanel: panel }

    // Enable/Action parameters
    property Fact _failsafeBatteryEnable:     controller.getParameterFact(-1, "FS_BATT_ENABLE")
    property Fact _failsafeEKFEnable:         controller.getParameterFact(-1, "FS_EKF_ACTION")
    property Fact _failsafeGCSEnable:         controller.getParameterFact(-1, "FS_GCS_ENABLE")
    property Fact _failsafeLeakEnable:        controller.getParameterFact(-1, "FS_LEAK_ENABLE")
    property Fact _failsafePilotEnable:       controller.getParameterFact(-1, "FS_PILOT_INPUT")
    property Fact _failsafePressureEnable:    controller.getParameterFact(-1, "FS_PRESS_ENABLE")
    property Fact _failsafeTemperatureEnable: controller.getParameterFact(-1, "FS_TEMP_ENABLE")

    // Threshold parameters
    property Fact _failsafePressureThreshold:    controller.getParameterFact(-1, "FS_PRESS_MAX")
    property Fact _failsafeTemperatureThreshold: controller.getParameterFact(-1, "FS_TEMP_MAX")
    property Fact _failsafePilotTimeout:         controller.getParameterFact(-1, "FS_PILOT_TIMEOUT")
    property Fact _failsafeLeakPin:              controller.getParameterFact(-1, "LEAK1_PIN")
    property Fact _failsafeLeakLogic:            controller.getParameterFact(-1, "LEAK1_LOGIC")
    property Fact _failsafeEKFThreshold:         controller.getParameterFact(-1, "FS_EKF_THRESH")
    property Fact _failsafeBatteryVoltage:       controller.getParameterFact(-1, "FS_BATT_VOLTAGE")
    property Fact _failsafeBatteryCapacity:      controller.getParameterFact(-1, "FS_BATT_MAH")

    property Fact _armingCheck: controller.getParameterFact(-1, "ARMING_CHECK")

    Column {
        anchors.fill:       parent

        VehicleSummaryRow {
            labelText: qsTr("Arming Checks:")
            valueText:  _armingCheck.value & 1 ? qsTr("Enabled") : qsTr("Some disabled")
        }
        VehicleSummaryRow {
            labelText: qsTr("GCS failsafe:")
            valueText: _failsafeGCSEnable.enumOrValueString
        }
        VehicleSummaryRow {
            labelText: qsTr("Leak failsafe:")
            valueText:  _failsafeLeakEnable.enumOrValueString
        }
        VehicleSummaryRow {
            labelText: qsTr("Battery failsafe:")
            valueText:  _failsafeBatteryEnable.enumOrValueString
        }
        VehicleSummaryRow {
            labelText: qsTr("EKF failsafe:")
            valueText:  _failsafeEKFEnable.enumOrValueString
        }
        VehicleSummaryRow {
            labelText: qsTr("Pilot Input failsafe:")
            valueText:  _failsafePilotEnable.enumOrValueString
        }
        VehicleSummaryRow {
            labelText: qsTr("Int. Temperature failsafe:")
            valueText:  _failsafeTemperatureEnable.enumOrValueString
        }
        VehicleSummaryRow {
            labelText: qsTr("Int. Pressure failsafe:")
            valueText:  _failsafePressureEnable.enumOrValueString
        }
    }
}

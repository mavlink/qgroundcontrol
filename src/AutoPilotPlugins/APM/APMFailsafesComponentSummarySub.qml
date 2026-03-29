import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

Item {
    anchors.fill: parent

    property bool _firmware34: globals.activeVehicle.versionCompare(3, 5, 0) < 0

    FactPanelController { id: controller; }

    property Fact _failsafeBatteryEnable:     controller.getParameterFact(-1, "BATT_FS_LOW_ACT", false)
    property Fact _failsafeEKFEnable:         controller.getParameterFact(-1, "FS_EKF_ACTION")
    property Fact _failsafeGCSEnable:         controller.getParameterFact(-1, "FS_GCS_ENABLE")
    property Fact _failsafeLeakEnable:        controller.getParameterFact(-1, "FS_LEAK_ENABLE")
    property Fact _failsafePilotEnable:       _firmware34 ? null : controller.getParameterFact(-1, "FS_PILOT_INPUT")
    property Fact _failsafeTemperatureEnable: controller.getParameterFact(-1, "FS_TEMP_ENABLE")
    property Fact _failsafePressureEnable:    controller.getParameterFact(-1, "FS_PRESS_ENABLE")

    Column {
        anchors.fill: parent

        VehicleSummaryRow {
            labelText: qsTr("GCS failsafe:")
            valueText: _failsafeGCSEnable.enumOrValueString
        }
        VehicleSummaryRow {
            labelText: qsTr("Leak failsafe:")
            valueText: _failsafeLeakEnable.enumOrValueString
        }
        VehicleSummaryRow {
            visible: !_firmware34
            labelText: qsTr("Battery failsafe:")
            valueText: {
                if (_firmware34) {
                    return "Firmware not supported"
                }
                if (!_failsafeBatteryEnable) {
                    return "Disabled"
                }
                return _failsafeBatteryEnable.enumOrValueString
            }
        }
        VehicleSummaryRow {
            visible: !_firmware34
            labelText: qsTr("EKF failsafe:")
            valueText: _firmware34 ? "" : _failsafeEKFEnable.enumOrValueString
        }
        VehicleSummaryRow {
            visible: !_firmware34
            labelText: qsTr("Pilot Input failsafe:")
            valueText: _firmware34 ? "" : _failsafePilotEnable.enumOrValueString
        }
        VehicleSummaryRow {
            labelText: qsTr("Int. Temperature failsafe:")
            valueText: _failsafeTemperatureEnable.enumOrValueString
        }
        VehicleSummaryRow {
            labelText: qsTr("Int. Pressure failsafe:")
            valueText: _failsafePressureEnable.enumOrValueString
        }
    }
}

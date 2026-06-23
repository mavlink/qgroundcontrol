import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

Item {
    implicitWidth: mainLayout.implicitWidth
    implicitHeight: mainLayout.implicitHeight
    width: parent.width

    FactPanelController { id: controller; }

    property Fact _batt1Monitor:            controller.getParameterFact(-1, "BATT_MONITOR")
    property Fact _batt2Monitor:            controller.getParameterFact(-1, "BATT2_MONITOR", false /* reportMissing */)
    property bool _batt2MonitorAvailable:   controller.parameterExists(-1, "BATT2_MONITOR")
    property bool _batt1MonitorEnabled:     _batt1Monitor.rawValue !== 0
    property bool _batt2MonitorEnabled:     _batt2MonitorAvailable && _batt2Monitor.rawValue !== 0

    property Fact _batt1FSLowAct:           controller.getParameterFact(-1, "BATT_FS_LOW_ACT", false /* reportMissing */)
    property Fact _batt1FSCritAct:          controller.getParameterFact(-1, "BATT_FS_CRT_ACT", false /* reportMissing */)
    property Fact _batt2FSLowAct:           controller.getParameterFact(-1, "BATT2_FS_LOW_ACT", false /* reportMissing */)
    property Fact _batt2FSCritAct:          controller.getParameterFact(-1, "BATT2_FS_CRT_ACT", false /* reportMissing */)
    property bool _batt1FSCritActAvailable: controller.parameterExists(-1, "BATT_FS_CRT_ACT")

    property bool _roverFirmware: controller.parameterExists(-1, "MODE1")

    ColumnLayout {
        id: mainLayout
        spacing: 0

        VehicleSummaryRow {
            labelText:  qsTr("Throttle failsafe:")
            valueText:  fact ? fact.enumStringValue : ""
            visible:    controller.vehicle.multiRotor

            property Fact fact: controller.getParameterFact(-1, "FS_THR_ENABLE", false /* reportMissing */)
        }

        VehicleSummaryRow {
            labelText:  qsTr("Throttle failsafe:")
            valueText:  fact ? fact.enumStringValue : ""
            visible:    controller.vehicle.fixedWing

            property Fact fact: controller.getParameterFact(-1, "THR_FAILSAFE", false /* reportMissing */)
        }

        VehicleSummaryRow {
            labelText:  qsTr("Throttle failsafe:")
            valueText:  fact ? fact.enumStringValue : ""
            visible:    _roverFirmware

            property Fact fact: controller.getParameterFact(-1, "FS_THR_ENABLE", false /* reportMissing */)
        }

        VehicleSummaryRow {
            labelText:  qsTr("Failsafe Action:")
            valueText:  fact ? fact.enumStringValue : ""
            visible:    _roverFirmware

            property Fact fact: controller.getParameterFact(-1, "FS_ACTION", false /* reportMissing */)
        }

        VehicleSummaryRow {
            labelText:  qsTr("Failsafe Crash Check:")
            valueText:  fact ? fact.enumStringValue : ""
            visible:    _roverFirmware

            property Fact fact: controller.getParameterFact(-1, "FS_CRASH_CHECK", false /* reportMissing */)
        }

        VehicleSummaryRow {
            labelText:  qsTr("Batt1 low failsafe:")
            valueText:  _batt1MonitorEnabled ? _batt1FSLowAct.enumStringValue : ""
            visible:    _batt1MonitorEnabled
        }

        VehicleSummaryRow {
            labelText:  qsTr("Batt1 critical failsafe:")
            valueText:  _batt1FSCritActAvailable ? _batt1FSCritAct.enumStringValue : ""
            visible:    _batt1FSCritActAvailable
        }

        VehicleSummaryRow {
            labelText:  qsTr("Batt2 low failsafe:")
            valueText:  _batt2MonitorEnabled ? _batt2FSLowAct.enumStringValue : ""
            visible:    _batt2MonitorEnabled
        }

        VehicleSummaryRow {
            labelText:  qsTr("Batt2 critical failsafe:")
            valueText:  _batt2MonitorEnabled ? _batt2FSCritAct.enumStringValue : ""
            visible:    _batt2MonitorEnabled
        }
    }
}

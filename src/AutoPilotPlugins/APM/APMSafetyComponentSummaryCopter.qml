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

    property Fact _failsafeBattLowAct:  controller.getParameterFact(-1, "r.BATT_FS_LOW_ACT")
    property Fact _failsafeThrEnable:   controller.getParameterFact(-1, "FS_THR_ENABLE")

    property Fact _fenceAction: controller.getParameterFact(-1, "FENCE_ACTION")
    property Fact _fenceEnable: controller.getParameterFact(-1, "FENCE_ENABLE")
    property Fact _fenceType:   controller.getParameterFact(-1, "FENCE_TYPE")

    property Fact _rtlAltFact:      controller.getParameterFact(-1, "RTL_ALT")
    property Fact _rtlLoitTimeFact: controller.getParameterFact(-1, "RTL_LOIT_TIME")
    property Fact _rtlAltFinalFact: controller.getParameterFact(-1, "RTL_ALT_FINAL")
    property Fact _landSpeedFact:   controller.getParameterFact(-1, "LAND_SPEED")

    property Fact _armingCheck: controller.getParameterFact(-1, "ARMING_CHECK")

    property bool _failsafeBattCritActAvailable:    controller.parameterExists(-1, "BATT_FS_CRT_ACT")
    property bool _failsafeBatt2LowActAvailable:    controller.parameterExists(-1, "BATT2_FS_LOW_ACT")
    property bool _failsafeBatt2CritActAvailable:   controller.parameterExists(-1, "BATT2_FS_CRT_ACT")

    property Fact _failsafeBattCritAct:             controller.getParameterFact(-1, "BATT_FS_CRT_ACT", false /* reportMissing */)
    property Fact _batt2Monitor:                    controller.getParameterFact(-1, "BATT2_MONITOR", false /* reportMissing */)
    property Fact _failsafeBatt2LowAct:             controller.getParameterFact(-1, "BATT2_FS_LOW_ACT", false /* reportMissing */)
    property Fact _failsafeBatt2CritAct:            controller.getParameterFact(-1, "BATT2_FS_CRT_ACT", false /* reportMissing */)

    Column {
        anchors.fill:       parent

        VehicleSummaryRow {
            labelText: qsTr("Arming Checks:")
            valueText:  _armingCheck.value & 1 ? qsTr("Enabled") : qsTr("Some disabled")
        }

        VehicleSummaryRow {
            labelText: qsTr("Throttle failsafe:")
            valueText:  _failsafeBattLowAct.enumStringValue
        }

        VehicleSummaryRow {
            labelText: qsTr("Batt low failsafe:")
            valueText:  _failsafeBattLowAct.enumStringValue
        }

        VehicleSummaryRow {
            labelText:  qsTr("Batt critical failsafe:")
            valueText:  _failsafeBattCritActAvailable ? _failsafeBattCritAct.enumStringValue : ""
            visible:    _failsafeBattCritActAvailable
        }

        VehicleSummaryRow {
            labelText: qsTr("Batt2 low failsafe:")
            valueText:  _failsafeBatt2LowActAvailable ? _failsafeBatt2LowAct.enumStringValue : ""
            visible:    _failsafeBatt2LowActAvailable
        }

        VehicleSummaryRow {
            labelText: qsTr("Batt2 critical failsafe:")
            valueText:  _failsafeBatt2CritActAvailable ? _failsafeBatt2CritAct.enumStringValue : ""
            visible:    _failsafeBatt2CritActAvailable
        }

        VehicleSummaryRow {
            labelText: qsTr("GeoFence:")
            valueText: _fenceEnable.value == 0 || _fenceType == 0 ?
                           qsTr("Disabled") :
                           (_fenceType.value == 1 ?
                                qsTr("Altitude") :
                                (_fenceType.value == 2 ? qsTr("Circle") : qsTr("Altitude,Circle")))
        }

        VehicleSummaryRow {
            labelText: qsTr("GeoFence:")
            valueText: _fenceAction.value == 0 ?
                           qsTr("Report only") :
                           (_fenceAction.value == 1 ? qsTr("RTL or Land") : qsTr("Unknown"))
            visible:    _fenceEnable.value != 0
        }

        VehicleSummaryRow {
            labelText: qsTr("RTL min alt:")
            valueText: _rtlAltFact.value == 0 ? qsTr("current") : _rtlAltFact.valueString + " " + _rtlAltFact.units
        }

        VehicleSummaryRow {
            labelText: qsTr("RTL loiter time:")
            valueText: _rtlLoitTimeFact.valueString + " " + _rtlLoitTimeFact.units
        }

        VehicleSummaryRow {
            labelText: qsTr("RTL final alt:")
            valueText: _rtlAltFinalFact.value == 0 ? qsTr("Land") : _rtlAltFinalFact.valueString + " " + _rtlAltFinalFact.units
        }

        VehicleSummaryRow {
            labelText: qsTr("Descent speed:")
            valueText: _landSpeedFact.valueString + " " + _landSpeedFact.units
        }
    }
}

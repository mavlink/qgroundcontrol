import QtQuick 2.3
import QtQuick.Controls 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0

Item {
    anchors.fill:   parent
    color:          qgcPal.windowShadeDark

    FactPanelController { id: controller; }

    property Fact _failsafeThrEnable:       controller.getParameterFact(-1, "FS_THR_ENABLE")

    property Fact _fenceAction:             controller.getParameterFact(-1, "FENCE_ACTION")
    property Fact _fenceEnable:             controller.getParameterFact(-1, "FENCE_ENABLE")
    property Fact _fenceType:               controller.getParameterFact(-1, "FENCE_TYPE")

    property Fact _rtlAltFact:              controller.getParameterFact(-1, "RTL_ALT")

    property Fact _armingCheck:             controller.getParameterFact(-1, "ARMING_CHECK")

    property Fact _batt1Monitor:            controller.getParameterFact(-1, "BATT_MONITOR")
    property Fact _batt2Monitor:            controller.getParameterFact(-1, "BATT2_MONITOR", false /* reportMissing */)
    property bool _batt2MonitorAvailable:   controller.parameterExists(-1, "BATT2_MONITOR")
    property bool _batt1MonitorEnabled:     _batt1Monitor.rawValue !== 0
    property bool _batt2MonitorEnabled:     _batt2MonitorAvailable && _batt2Monitor.rawValue !== 0

    property Fact _batt1FSLowAct:           controller.getParameterFact(-1, "r.BATT_FS_LOW_ACT", false /* reportMissing */)
    property Fact _batt1FSCritAct:          controller.getParameterFact(-1, "BATT_FS_CRT_ACT", false /* reportMissing */)
    property Fact _batt2FSLowAct:           controller.getParameterFact(-1, "BATT2_FS_LOW_ACT", false /* reportMissing */)
    property Fact _batt2FSCritAct:          controller.getParameterFact(-1, "BATT2_FS_CRT_ACT", false /* reportMissing */)
    property bool _batt1FSCritActAvailable: controller.parameterExists(-1, "BATT_FS_CRT_ACT")

    Column {
        anchors.fill:       parent

        VehicleSummaryRow {
            labelText: qsTr("Arming Checks:")
            valueText: _armingCheck.value & 1 ? qsTr("Enabled") : qsTr("Some disabled")
        }

        VehicleSummaryRow {
            labelText: qsTr("Throttle failsafe:")
            valueText: _failsafeThrEnable.enumStringValue
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
    }
}

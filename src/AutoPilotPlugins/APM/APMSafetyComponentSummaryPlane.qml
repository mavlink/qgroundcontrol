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

    property Fact _failsafeBattMah:     controller.getParameterFact(-1, "FS_BATT_MAH")
    property Fact _failsafeBattVoltage: controller.getParameterFact(-1, "FS_BATT_VOLTAGE")
    property Fact _failsafeThrEnable:   controller.getParameterFact(-1, "THR_FAILSAFE")
    property Fact _failsafeThrValue:    controller.getParameterFact(-1, "THR_FS_VALUE")
    property Fact _failsafeGCSEnable:   controller.getParameterFact(-1, "FS_GCS_ENABL")

    property Fact _rtlAltFact: controller.getParameterFact(-1, "ALT_HOLD_RTL")

    Column {
        anchors.fill:       parent

        VehicleSummaryRow {
            labelText: qsTr("Throttle failsafe:")
            valueText:  _failsafeThrEnable.value != 0 ? _failsafeThrValue.valueString : qsTr("Disabled")
        }

        VehicleSummaryRow {
            labelText: qsTr("Voltage failsafe:")
            valueText:  _failsafeBattVoltage.value == 0 ? qsTr("Disabled") : _failsafeBattVoltage.valueString + " " + _failsafeBattVoltage.units
        }

        VehicleSummaryRow {
            labelText: qsTr("mAh failsafe:")
            valueText:  _failsafeBattMah.value == 0 ? qsTr("Disabled") : _failsafeBattMah.valueString + " " + _failsafeBattMah.units
        }

        VehicleSummaryRow {
            labelText: qsTr("RTL min alt:")
            valueText: _rtlAltFact.value < 0 ? qsTr("current") : _rtlAltFact.valueString + " " + _rtlAltFact.units
        }
    }
}

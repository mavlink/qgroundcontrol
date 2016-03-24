import QtQuick 2.2
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

    property Fact _failsafeBattMah:     controller.getParameterFact(-1, "FS_BATT_MAH")
    property Fact _failsafeBattVoltage: controller.getParameterFact(-1, "FS_BATT_VOLTAGE")
    property Fact _failsafeThrEnable:   controller.getParameterFact(-1, "THR_FAILSAFE")
    property Fact _failsafeThrValue:    controller.getParameterFact(-1, "THR_FS_VALUE")
    property Fact _failsafeGCSEnable:   controller.getParameterFact(-1, "FS_GCS_ENABL")

    property Fact _rtlAltFact: controller.getParameterFact(-1, "ALT_HOLD_RTL")

    Column {
        anchors.fill:       parent
        anchors.margins:    8

        VehicleSummaryRow {
            labelText: qsTr("Throttle failsafe:")
            valueText:  _failsafeThrEnable.value != 0 ? _failsafeThrValue.valueString : qsTr("Disabled")
        }

        VehicleSummaryRow {
            labelText: qsTr("Voltage failsafe:")
            valueText:  _failsafeBattVoltage.value == 0 ? qsTr("Disabled") : _failsafeBattVoltage.valueString
        }

        VehicleSummaryRow {
            labelText: qsTr("mAh failsafe:")
            valueText:  _failsafeBattMah.value == 0 ? qsTr("Disabled") : _failsafeBattMah.valueString
        }

        VehicleSummaryRow {
            labelText: qsTr("RTL min alt:")
            valueText: _rtlAltFact.value < 0 ? qsTr("current") : _rtlAltFact.valueString
        }
    }
}

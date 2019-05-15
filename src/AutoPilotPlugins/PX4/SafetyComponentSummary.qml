import QtQuick 2.3
import QtQuick.Controls 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0

Item {
    anchors.fill:   parent

    FactPanelController { id: controller; }

    property Fact   returnAltFact:      controller.getParameterFact(-1, "RTL_RETURN_ALT")
    property Fact   _descendAltFact:    controller.getParameterFact(-1, "RTL_DESCEND_ALT")
    property Fact   landDelayFact:      controller.getParameterFact(-1, "RTL_LAND_DELAY")
    property Fact   commRCLossFact:     controller.getParameterFact(-1, "COM_RC_LOSS_T")
    property Fact   lowBattAction:      controller.getParameterFact(-1, "COM_LOW_BAT_ACT")
    property Fact   rcLossAction:       controller.getParameterFact(-1, "NAV_RCL_ACT")
    property Fact   dataLossAction:     controller.getParameterFact(-1, "NAV_DLL_ACT")
    property Fact   _rtlLandDelayFact:  controller.getParameterFact(-1, "RTL_LAND_DELAY")
    property int    _rtlLandDelayValue: _rtlLandDelayFact.value

    Column {
        anchors.fill:       parent

        VehicleSummaryRow {
            labelText: qsTr("Low Battery Failsafe")
            valueText: lowBattAction ? lowBattAction.enumStringValue : ""
        }

        VehicleSummaryRow {
            labelText: qsTr("RC Loss Failsafe")
            valueText: rcLossAction ? rcLossAction.enumStringValue : ""
        }

        VehicleSummaryRow {
            labelText: qsTr("RC Loss Timeout")
            valueText: commRCLossFact ? commRCLossFact.valueString + " " + commRCLossFact.units : ""
        }

        VehicleSummaryRow {
            labelText: qsTr("Data Link Loss Failsafe")
            valueText: dataLossAction ? dataLossAction.enumStringValue : ""
        }

        VehicleSummaryRow {
            labelText: qsTr("RTL Climb To")
            valueText: returnAltFact ? returnAltFact.valueString + " " + returnAltFact.units : ""
        }

        VehicleSummaryRow {
            labelText: qsTr("RTL, Then")
            valueText: _rtlLandDelayValue === 0 ?
                           qsTr("Land immediately") :
                           (_rtlLandDelayValue < 0 ?
                                qsTr("Loiter and do not land") :
                                qsTr("Loiter and land after specified time"))

        }

        VehicleSummaryRow {
            labelText: qsTr("Loiter Alt")
            valueText: _descendAltFact.valueString + " " + _descendAltFact.units
            visible:    _rtlLandDelayValue !== 0
        }

        VehicleSummaryRow {
            labelText: qsTr("Land Delay")
            valueText: _rtlLandDelayValue + " " + _rtlLandDelayFact.units
            visible:    _rtlLandDelayValue > 0
        }
    }
}

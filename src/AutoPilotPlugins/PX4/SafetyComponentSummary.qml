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

    property Fact returnAltFact:    controller.getParameterFact(-1, "RTL_RETURN_ALT")
    property Fact descendAltFact:   controller.getParameterFact(-1, "RTL_DESCEND_ALT")
    property Fact landDelayFact:    controller.getParameterFact(-1, "RTL_LAND_DELAY")
    property Fact commRCLossFact:   controller.getParameterFact(-1, "COM_RC_LOSS_T")
    property Fact lowBattAction:    controller.getParameterFact(-1, "COM_LOW_BAT_ACT")
    property Fact rcLossAction:     controller.getParameterFact(-1, "NAV_RCL_ACT")
    property Fact dataLossAction:   controller.getParameterFact(-1, "NAV_DLL_ACT")

    Column {
        anchors.fill:       parent
        anchors.margins:    8

        VehicleSummaryRow {
            labelText: qsTr("RTL min alt:")
            valueText: returnAltFact ? returnAltFact.valueString : ""
        }

        VehicleSummaryRow {
            labelText: qsTr("RTL home alt:")
            valueText: descendAltFact ? descendAltFact.valueString : ""
        }

        VehicleSummaryRow {
            labelText: qsTr("RC loss RTL (seconds):")
            valueText: commRCLossFact ? commRCLossFact.valueString : ""
        }

        VehicleSummaryRow {
            labelText: qsTr("RC loss action:")
            valueText: rcLossAction ? rcLossAction.enumStringValue : ""
        }

        VehicleSummaryRow {
            labelText: qsTr("Link loss action:")
            valueText: dataLossAction ? dataLossAction.enumStringValue : ""
        }

        VehicleSummaryRow {
            labelText: qsTr("Low battery action:")
            valueText: lowBattAction ? lowBattAction.enumStringValue : ""
        }

    }
}

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
    property Fact commDLLossFact:   controller.getParameterFact(-1, "COM_DL_LOSS_EN")
    property Fact commRCLossFact:   controller.getParameterFact(-1, "COM_RC_LOSS_T")

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
            labelText: qsTr("RTL loiter delay:")
            valueText: landDelayFact ? (landDelayFact.value < 0 ? qsTr("Disabled") : landDelayFact.valueString) : ""
        }

        VehicleSummaryRow {
            labelText: qsTr("Telemetry loss RTL:")
            valueText: commDLLossFact ? (commDLLossFact.value != -1 ? qsTr("Disabled") : commDLLossFact.valueString) : ""
        }

        VehicleSummaryRow {
            labelText: qsTr("RC loss RTL (seconds):")
            valueText: commRCLossFact ? commRCLossFact.valueString : ""
        }
    }
}

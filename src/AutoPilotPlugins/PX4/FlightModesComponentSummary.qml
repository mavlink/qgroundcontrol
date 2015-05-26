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

    property Fact modeSwFact:   controller.getParameterFact(-1, "RC_MAP_MODE_SW")
    property Fact posCtlSwFact: controller.getParameterFact(-1, "RC_MAP_POSCTL_SW")
    property Fact loiterSwFact: controller.getParameterFact(-1, "RC_MAP_LOITER_SW")
    property Fact returnSwFact: controller.getParameterFact(-1, "RC_MAP_RETURN_SW")

    Column {
        anchors.fill:       parent
        anchors.margins:    8

        VehicleSummaryRow {
            labelText: "Mode switch:"
            valueText: modeSwFact.value == 0 ? "Setup required" : modeSwFact.valueString
        }

        VehicleSummaryRow {
            labelText: "Position Ctl switch:"
            valueText: posCtlSwFact.value == 0 ? "Disabled" : posCtlSwFact.valueString
        }

        VehicleSummaryRow {
            labelText: "Loiter switch:"
            valueText: loiterSwFact.value == 0 ? "Disabled" : loiterSwFact.valueString
        }

        VehicleSummaryRow {
            labelText: "Return switch:"
            valueText: returnSwFact.value == 0 ? "Disabled" : returnSwFact.valueString
        }
    }
}

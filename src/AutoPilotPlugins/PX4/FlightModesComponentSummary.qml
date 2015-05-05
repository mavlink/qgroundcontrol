import QtQuick 2.2
import QtQuick.Controls 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Controls 1.0

FactPanel {
    anchors.fill: parent

    Fact { id: modeSwFact;      name: "RC_MAP_MODE_SW";     onFactMissing: showMissingFactOverlay(name) }
    Fact { id: posCtlSwFact;    name: "RC_MAP_POSCTL_SW";   onFactMissing: showMissingFactOverlay(name) }
    Fact { id: loiterSwFact;    name: "RC_MAP_LOITER_SW";   onFactMissing: showMissingFactOverlay(name) }
    Fact { id: returnSwFact;    name: "RC_MAP_RETURN_SW";   onFactMissing: showMissingFactOverlay(name) }

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

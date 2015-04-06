import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Controls 1.0

Column {
    Fact { id: returnAltFact;   name: "RTL_RETURN_ALT" }
    Fact { id: descendAltFact;  name: "RTL_DESCEND_ALT" }
    Fact { id: landDelayFact;   name: "RTL_LAND_DELAY" }
    Fact { id: commDLLossFact;  name: "COM_DL_LOSS_EN" }
    Fact { id: commRCLossFact;  name: "COM_RC_LOSS_T" }

    anchors.fill:       parent
    anchors.margins:    8

    VehicleSummaryRow {
        labelText: "RTL min alt:"
        valueText: returnAltFact.valueString
    }

    VehicleSummaryRow {
        labelText: "RTL home alt:"
        valueText: descendAltFact.valueString
    }

    VehicleSummaryRow {
        labelText: "RTL loiter delay:"
        valueText: landDelayFact.value < 0 ? "Disabled" : landDelayFact.valueString
    }

    VehicleSummaryRow {
        labelText: "Telemetry loss RTL:"
        valueText: commDLLossFact.value != -1 ? "Disabled" : commDLLossFact.valueString
    }

    VehicleSummaryRow {
        labelText: "RC loss RTL (seconds):"
        valueText: commRCLossFact.valueString
    }
}

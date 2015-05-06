import QtQuick 2.2
import QtQuick.Controls 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Controls 1.0

FactPanel {
    anchors.fill: parent

    Fact { id: mapRollFact;     name: "RC_MAP_ROLL";        onFactMissing: showMissingFactOverlay(name) }
    Fact { id: mapPitchFact;    name: "RC_MAP_PITCH";       onFactMissing: showMissingFactOverlay(name) }
    Fact { id: mapYawFact;      name: "RC_MAP_YAW";         onFactMissing: showMissingFactOverlay(name) }
    Fact { id: mapThrottleFact; name: "RC_MAP_THROTTLE";    onFactMissing: showMissingFactOverlay(name) }
    Fact { id: mapFlapsFact;    name: "RC_MAP_FLAPS";       onFactMissing: showMissingFactOverlay(name) }
    Fact { id: mapAux1Fact;     name: "RC_MAP_AUX1";        onFactMissing: showMissingFactOverlay(name) }
    Fact { id: mapAux2Fact;     name: "RC_MAP_AUX2";        onFactMissing: showMissingFactOverlay(name) }

    Column {
        anchors.fill:       parent
        anchors.margins:    8

        VehicleSummaryRow {
            labelText: "Roll:"
            valueText: mapRollFact.value == 0 ? "Setup required" : mapRollFact.valueString
        }

        VehicleSummaryRow {
            labelText: "Pitch:"
            valueText: mapPitchFact.value == 0 ? "Setup required" : mapPitchFact.valueString
        }

        VehicleSummaryRow {
            labelText: "Yaw:"
            valueText: mapYawFact.value == 0 ? "Setup required" : mapYawFact.valueString
        }

        VehicleSummaryRow {
            labelText: "Throttle:"
            valueText: mapThrottleFact.value == 0 ? "Setup required" : mapThrottleFact.valueString
        }

        VehicleSummaryRow {
            labelText: "Flaps:"
            valueText: mapFlapsFact.value == 0 ? "Disabled" : mapFlapsFact.valueString
        }

        VehicleSummaryRow {
            labelText: "Aux1:"
            valueText: mapAux1Fact.value == 0 ? "Disabled" : mapAux1Fact.valueString
        }

        VehicleSummaryRow {
            labelText: "Aux2:"
            valueText: mapAux2Fact.value == 0 ? "Disabled" : mapAux2Fact.valueString
        }
    }
}

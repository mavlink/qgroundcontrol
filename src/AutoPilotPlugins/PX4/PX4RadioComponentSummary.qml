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

    property Fact mapRollFact:      controller.getParameterFact(-1, "RC_MAP_ROLL")
    property Fact mapPitchFact:     controller.getParameterFact(-1, "RC_MAP_PITCH")
    property Fact mapYawFact:       controller.getParameterFact(-1, "RC_MAP_YAW")
    property Fact mapThrottleFact:  controller.getParameterFact(-1, "RC_MAP_THROTTLE")
    property Fact mapFlapsFact:     controller.getParameterFact(-1, "RC_MAP_FLAPS")
    property Fact mapAux1Fact:      controller.getParameterFact(-1, "RC_MAP_AUX1")
    property Fact mapAux2Fact:      controller.getParameterFact(-1, "RC_MAP_AUX2")

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

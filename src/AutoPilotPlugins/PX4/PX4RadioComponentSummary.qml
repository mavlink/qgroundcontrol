import QtQuick 2.3
import QtQuick.Controls 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0

Item {
    anchors.fill:   parent

    FactPanelController { id: controller; }

    property Fact mapRollFact:      controller.getParameterFact(-1, "RC_MAP_ROLL")
    property Fact mapPitchFact:     controller.getParameterFact(-1, "RC_MAP_PITCH")
    property Fact mapYawFact:       controller.getParameterFact(-1, "RC_MAP_YAW")
    property Fact mapThrottleFact:  controller.getParameterFact(-1, "RC_MAP_THROTTLE")
    property Fact mapFlapsFact:     controller.getParameterFact(-1, "RC_MAP_FLAPS")
    property Fact mapAux1Fact:      controller.getParameterFact(-1, "RC_MAP_AUX1")
    property Fact mapAux2Fact:      controller.getParameterFact(-1, "RC_MAP_AUX2")

    Column {
        anchors.fill:       parent

        VehicleSummaryRow {
            labelText: qsTr("Roll")
            valueText: mapRollFact ? (mapRollFact.value === 0 ? qsTr("Setup required") : mapRollFact.valueString) : ""
        }

        VehicleSummaryRow {
            labelText: qsTr("Pitch")
            valueText: mapPitchFact ? (mapPitchFact.value === 0 ? qsTr("Setup required") : mapPitchFact.valueString) : ""
        }

        VehicleSummaryRow {
            labelText: qsTr("Yaw")
            valueText: mapYawFact ? (mapYawFact.value === 0 ? qsTr("Setup required") : mapYawFact.valueString) : ""
        }

        VehicleSummaryRow {
            labelText: qsTr("Throttle")
            valueText: mapThrottleFact ? (mapThrottleFact.value === 0 ? qsTr("Setup required") : mapThrottleFact.valueString) : ""
        }

        VehicleSummaryRow {
            labelText:  qsTr("Flaps")
            valueText:  mapFlapsFact ? (mapFlapsFact.value === 0 ? qsTr("Disabled") : mapFlapsFact.valueString) : ""
            visible:    !controller.vehicle.multiRotor
        }

        VehicleSummaryRow {
            labelText: qsTr("Aux1")
            valueText: mapAux1Fact ? (mapAux1Fact.value === 0 ? qsTr("Disabled") : mapAux1Fact.valueString) : ""
        }

        VehicleSummaryRow {
            labelText: qsTr("Aux2")
            valueText: mapAux2Fact ? (mapAux2Fact.value === 0 ? qsTr("Disabled") : mapAux2Fact.valueString) : ""
        }
    }
}

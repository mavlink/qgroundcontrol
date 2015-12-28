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

    property Fact mapRollFact:      controller.getParameterFact(-1, "RCMAP_ROLL")
    property Fact mapPitchFact:     controller.getParameterFact(-1, "RCMAP_PITCH")
    property Fact mapYawFact:       controller.getParameterFact(-1, "RCMAP_YAW")
    property Fact mapThrottleFact:  controller.getParameterFact(-1, "RCMAP_THROTTLE")

    Column {
        anchors.fill:       parent
        anchors.margins:    8

        VehicleSummaryRow {
            labelText: "Roll:"
            valueText: mapRollFact.value == 0 ? "Setup required" : "Channel " + mapRollFact.valueString
        }

        VehicleSummaryRow {
            labelText: "Pitch:"
            valueText: mapPitchFact.value == 0 ? "Setup required" : "Channel " + mapPitchFact.valueString
        }

        VehicleSummaryRow {
            labelText: "Yaw:"
            valueText: mapYawFact.value == 0 ? "Setup required" : "Channel " + mapYawFact.valueString
        }

        VehicleSummaryRow {
            labelText: "Throttle:"
            valueText: mapThrottleFact.value == 0 ? "Setup required" : "Channel " + mapThrottleFact.valueString
        }
    }
}

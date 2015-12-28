import QtQuick          2.5
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

    property Fact _mountRCInTilt:   controller.getParameterFact(-1, "MNT_RC_IN_TILT")
    property Fact _mountRCInRoll:   controller.getParameterFact(-1, "MNT_RC_IN_ROLL")
    property Fact _mountRCInPan:    controller.getParameterFact(-1, "MNT_RC_IN_PAN")
    property Fact _mountType:       controller.getParameterFact(-1, "MNT_TYPE")

    Column {
        anchors.fill:       parent
        anchors.margins:    8

        VehicleSummaryRow {
            labelText: "Gimbal type:"
            valueText:  _mountType.enumStringValue
        }

        VehicleSummaryRow {
            labelText: "Tilt input channel:"
            valueText:  _mountRCInTilt.enumStringValue
        }

        VehicleSummaryRow {
            labelText: "Pan input channel:"
            valueText:  _mountRCInPan.enumStringValue
        }

        VehicleSummaryRow {
            labelText: "Roll input channel:"
            valueText:  _mountRCInRoll.enumStringValue
        }
    }
}

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

    property Fact _rtlAltFact:      controller.getParameterFact(-1, "RTL_ALT")
    property Fact _rtlLoitTimeFact: controller.getParameterFact(-1, "RTL_LOIT_TIME")
    property Fact _rtlAltFinalFact: controller.getParameterFact(-1, "RTL_ALT_FINAL")
    property Fact _landSpeedFact:   controller.getParameterFact(-1, "LAND_SPEED")

    Column {
        anchors.fill:       parent
        anchors.margins:    8

        VehicleSummaryRow {
            labelText: "RTL min alt:"
            valueText: _rtlAltFact.value == 0 ? "current" : _rtlAltFact.valueString
        }

        VehicleSummaryRow {
            labelText: "RTL loiter time:"
            valueText: _rtlLoitTimeFact.valueString
        }

        VehicleSummaryRow {
            labelText: "RTL final alt:"
            valueText: _rtlAltFinalFact.value == 0 ? "Land" : _rtlAltFinalFact.valueString
        }

        VehicleSummaryRow {
            labelText: "Descent speed:"
            valueText: _landSpeedFact.valueString
        }
    }
}

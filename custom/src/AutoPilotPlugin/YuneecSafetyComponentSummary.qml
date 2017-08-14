import QtQuick 2.3
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

    property Fact _returnAltFact:   controller.getParameterFact(-1, "RTL_RETURN_ALT")
    property Fact _fenceAction:     controller.getParameterFact(-1, "GF_ACTION")
    property Fact _fenceRadius:     controller.getParameterFact(-1, "GF_MAX_HOR_DIST")
    property Fact _fenceAlt:        controller.getParameterFact(-1, "GF_MAX_VER_DIST")

    Column {
        anchors.fill:       parent
        VehicleSummaryRow {
            labelText: qsTr("RTL min alt:")
            valueText: _returnAltFact ? _returnAltFact.valueString + " " + _returnAltFact.units : ""
        }
        VehicleSummaryRow {
            labelText: qsTr("Fence Radius:")
            valueText: _fenceRadius ? _fenceRadius.valueString : ""
            visible:   _fenceRadius ? _fenceRadius.value > 0 : false
        }
        VehicleSummaryRow {
            labelText: qsTr("Max Altitude:")
            valueText: _fenceAlt ? _fenceAlt.valueString : ""
            visible:   _fenceAlt ? _fenceAlt.value > 0 : false
        }
    }
}

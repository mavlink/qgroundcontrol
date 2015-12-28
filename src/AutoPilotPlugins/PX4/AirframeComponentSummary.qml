import QtQuick 2.2
import QtQuick.Controls 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Controllers 1.0
import QGroundControl.Palette 1.0

FactPanel {
    id:             panel
    anchors.fill:   parent
    color:          qgcPal.windowShadeDark

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }
    AirframeComponentController { id: controller; factPanel: panel }

    property Fact sysIdFact:        controller.getParameterFact(-1, "MAV_SYS_ID")
    property Fact sysAutoStartFact: controller.getParameterFact(-1, "SYS_AUTOSTART")

    property bool autoStartSet: sysAutoStartFact.value != 0

    Column {
        anchors.fill: parent
        anchors.margins: 8

        VehicleSummaryRow {
            labelText: "System ID:"
            valueText: sysIdFact.valueString
        }

        VehicleSummaryRow {
            labelText: "Airframe type:"
            valueText: autoStartSet ? controller.currentAirframeType : "Setup required"
        }

        VehicleSummaryRow {
            labelText: "Vehicle:"
            valueText: autoStartSet ? controller.currentVehicleName : "Setup required"
        }
    }
}
import QtQuick 2.2
import QtQuick.Controls 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Controllers 1.0

FactPanel {
    id:             panel
    anchors.fill:   parent

    AirframeComponentController { id: controller; factPanel: panel }

    Fact { id: sysIdFact;           name: "MAV_SYS_ID";     onFactMissing: showMissingFactOverlay(name) }
    Fact { id: sysAutoStartFact;    name: "SYS_AUTOSTART";  onFactMissing: showMissingFactOverlay(name) }

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
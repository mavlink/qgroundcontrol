import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Controllers 1.0

Column {
    Fact { id: sysIdFact;           name: "MAV_SYS_ID" }
    Fact { id: sysAutoStartFact;    name: "SYS_AUTOSTART" }

    property bool autoStartSet: sysAutoStartFact.value != 0

    anchors.fill: parent
    anchors.margins: 8

    AirframeComponentController { id: controller }

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

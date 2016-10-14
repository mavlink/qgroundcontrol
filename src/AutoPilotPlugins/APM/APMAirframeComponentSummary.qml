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
    APMAirframeComponentController {
        id:         controller
        factPanel:  panel
    }

    property Fact sysIdFact:        controller.getParameterFact(-1, "FRAME")

    Column {
        anchors.fill:       parent
        VehicleSummaryRow {
            id: nameRow;
            labelText: qsTr("Frame Type:")
            valueText: controller.currentAirframeTypeName() + " " + (sysIdFact.valueString === "0" ? "Plus"
                     : sysIdFact.valueString === "1" ? "X"
                     : sysIdFact.valueString === "2" ? "V"
                     : sysIdFact.valueString == "3" ? "H"
                     : /* Fact.value == 10 */  "New Y6");

        }

        VehicleSummaryRow {
            labelText: qsTr("Firmware Version:")
            valueText: activeVehicle.firmwareMajorVersion == -1 ? qsTr("Unknown") : activeVehicle.firmwareMajorVersion + "." + activeVehicle.firmwareMinorVersion + "." + activeVehicle.firmwarePatchVersion + activeVehicle.firmwareVersionTypeString
        }
    }
}

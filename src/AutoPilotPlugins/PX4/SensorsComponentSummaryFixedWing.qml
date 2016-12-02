import QtQuick 2.2
import QtQuick.Controls 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0

/*
    IMPORTANT NOTE: Any changes made here must also be made to SensorsComponentSummary.qml
*/

FactPanel {
    id:             panel
    anchors.fill:   parent
    color:          qgcPal.windowShadeDark

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }
    FactPanelController { id: controller; factPanel: panel }

    property Fact mag0IdFact:       controller.getParameterFact(-1, "CAL_MAG0_ID")
    property Fact gyro0IdFact:      controller.getParameterFact(-1, "CAL_GYRO0_ID")
    property Fact accel0IdFact:     controller.getParameterFact(-1, "CAL_ACC0_ID")
    property Fact dpressOffFact:    controller.getParameterFact(-1, "SENS_DPRES_OFF")

    Column {
        anchors.fill:       parent

        VehicleSummaryRow {
            labelText: qsTr("Compass:")
            valueText: mag0IdFact ? (mag0IdFact.value  === 0 ? qsTr("Setup required") : qsTr("Ready")) : ""
        }

        VehicleSummaryRow {
            labelText: qsTr("Gyro:")
            valueText: gyro0IdFact ? (gyro0IdFact.value === 0 ? qsTr("Setup required") : qsTr("Ready")) : ""
        }

        VehicleSummaryRow {
            labelText: qsTr("Accelerometer:")
            valueText: accel0IdFact ? (accel0IdFact.value === 0 ? qsTr("Setup required") : qsTr("Ready")) : ""
        }

        VehicleSummaryRow {
            labelText: qsTr("Airspeed:")
            valueText: dpressOffFact ? (dpressOffFact.value === 0 ? qsTr("Setup required") : qsTr("Ready")) : ""
        }
    }
}

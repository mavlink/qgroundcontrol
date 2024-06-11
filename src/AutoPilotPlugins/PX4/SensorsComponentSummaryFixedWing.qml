import QtQuick
import QtQuick.Controls

import QGroundControl.FactSystem
import QGroundControl.FactControls
import QGroundControl.Controls
import QGroundControl.Palette

/*
    IMPORTANT NOTE: Any changes made here must also be made to SensorsComponentSummary.qml
*/

Item {
    anchors.fill:   parent

    FactPanelController { id: controller; }

    property Fact mag0IdFact:           controller.getParameterFact(-1, "CAL_MAG0_ID")
    property Fact gyro0IdFact:          controller.getParameterFact(-1, "CAL_GYRO0_ID")
    property Fact accel0IdFact:         controller.getParameterFact(-1, "CAL_ACC0_ID")

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
            labelText:  qsTr("Airspeed:")
            visible:    vehicleComponent.airspeedCalSupported
            valueText:  vehicleComponent.airspeedCalRequired ? qsTr("Setup required") : qsTr("Ready")
        }
    }
}

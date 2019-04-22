import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.4

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0

/*
    IMPORTANT NOTE: Any changes made here must also be made to SensorsComponentSummary.qml
*/

Item {
    anchors.fill:   parent

    FactPanelController { id: controller; }

    property Fact mag0IdFact:   controller.getParameterFact(-1, "CAL_MAG0_ID")
    property Fact mag1IdFact:   controller.getParameterFact(-1, "CAL_MAG1_ID")
    property Fact mag2IdFact:   controller.getParameterFact(-1, "CAL_MAG2_ID")
    property Fact gyro0IdFact:  controller.getParameterFact(-1, "CAL_GYRO0_ID")
    property Fact accel0IdFact: controller.getParameterFact(-1, "CAL_ACC0_ID")

    Column {
        anchors.fill:       parent

        VehicleSummaryRow {
            labelText: qsTr("Compass 0")
            valueText: mag0IdFact ? (mag0IdFact.value === 0 ? qsTr("Setup required") : qsTr("Ready")) : ""
        }

        VehicleSummaryRow {
            labelText:  qsTr("Compass 1")
            visible:    mag1IdFact.value !== 0
            valueText:  qsTr("Ready")
        }

        VehicleSummaryRow {
            labelText:  qsTr("Compass 2")
            visible:    mag2IdFact.value !== 0
            valueText:  qsTr("Ready")
        }

        VehicleSummaryRow {
            labelText: qsTr("Gyro")
            valueText: gyro0IdFact ? (gyro0IdFact.value === 0 ? qsTr("Setup required") : qsTr("Ready")) : ""
        }

        VehicleSummaryRow {
            labelText: qsTr("Accelerometer")
            valueText: accel0IdFact ? (accel0IdFact.value === 0 ? qsTr("Setup required") : qsTr("Ready")) : ""
        }
    }
}

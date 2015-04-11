import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.Controls 1.0

/*
    IMPORTANT NOTE: Any changes made here must also be made to SensorsComponentSummary.qml
*/

Column {
    Fact { id: mag0IdFact;      name: "CAL_MAG0_ID" }
    Fact { id: gyro0IdFact;     name: "CAL_GYRO0_ID" }
    Fact { id: accel0IdFact;    name: "CAL_ACC0_ID" }
    Fact { id: dPressOffFact;   name: "SENS_DPRES_OFF" }

    anchors.fill:       parent
    anchors.margins:    8

    VehicleSummaryRow {
        labelText: "Compass:"
        valueText: mag0IdFact.value  == 0 ? "Setup required" : "Ready"
    }

    VehicleSummaryRow {
        labelText: "Gyro:"
        valueText: gyro0IdFact.value  == 0 ? "Setup required" : "Ready"
    }

    VehicleSummaryRow {
        labelText: "Accelerometer:"
        valueText: accel0IdFact.value  == 0 ? "Setup required" : "Ready"
    }

    VehicleSummaryRow {
        labelText: "Airspeed:"
        valueText: dPressOffFact.value  == 0 ? "Setup required" : "Ready"
    }
}

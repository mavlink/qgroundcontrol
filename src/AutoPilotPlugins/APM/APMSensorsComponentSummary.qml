import QtQuick                  2.5
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controllers   1.0

/*
    IMPORTANT NOTE: Any changes made here must also be made to SensorsComponentSummary.qml
*/

FactPanel {
    id:             panel
    anchors.fill:   parent
    color:          qgcPal.windowShadeDark

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }
    APMSensorsComponentController { id: controller; factPanel: panel }

    property bool accelCalNeeded:   controller.accelSetupNeeded
    property bool compassCalNeeded: controller.compassSetupNeeded

    Column {
        anchors.fill:       parent
        anchors.margins:    8

        VehicleSummaryRow {
            labelText: "Compass:"
            valueText: compassCalNeeded ? "Setup required" : "Ready"
        }

        VehicleSummaryRow {
            labelText: "Accelerometer:"
            valueText: accelCalNeeded ? "Setup required" : "Ready"
        }
    }
}

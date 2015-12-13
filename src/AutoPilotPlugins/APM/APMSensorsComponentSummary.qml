import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

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

    property Fact ins_accoffs_x:    controller.getParameterFact(-1, "INS_ACCOFFS_X")
    property Fact ins_accoffs_y:    controller.getParameterFact(-1, "INS_ACCOFFS_Y")
    property Fact ins_accoffs_z:    controller.getParameterFact(-1, "INS_ACCOFFS_Z")

    property Fact compass_ofs_x:    controller.getParameterFact(-1, "COMPASS_OFS_X")
    property Fact compass_ofs_y:    controller.getParameterFact(-1, "COMPASS_OFS_Y")
    property Fact compass_ofs_z:    controller.getParameterFact(-1, "COMPASS_OFS_Z")

    property bool accelCalNeeded:   ins_accoffs_x.value == 0 && ins_accoffs_y.value == 0 && ins_accoffs_z.value == 0
    property bool compassCalNeeded: compass_ofs_x.value == 0 && compass_ofs_y.value == 0 && compass_ofs_y.value == 0

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

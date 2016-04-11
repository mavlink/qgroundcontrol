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

    property Fact compass1IdFact:   controller.getParameterFact(-1, "COMPASS_DEV_ID")
    property Fact compass2IdFact:   controller.getParameterFact(-1, "COMPASS_DEV_ID2")
    property Fact compass3IdFact:   controller.getParameterFact(-1, "COMPASS_DEV_ID3")

    property Fact compass1OfsXFact: controller.getParameterFact(-1, "COMPASS_OFS_X")
    property Fact compass1OfsYFact: controller.getParameterFact(-1, "COMPASS_OFS_Y")
    property Fact compass1OfsZFact: controller.getParameterFact(-1, "COMPASS_OFS_Z")
    property Fact compass2OfsXFact: controller.getParameterFact(-1, "COMPASS_OFS2_X")
    property Fact compass2OfsYFact: controller.getParameterFact(-1, "COMPASS_OFS2_Y")
    property Fact compass2OfsZFact: controller.getParameterFact(-1, "COMPASS_OFS2_Z")
    property Fact compass3OfsXFact: controller.getParameterFact(-1, "COMPASS_OFS3_X")
    property Fact compass3OfsYFact: controller.getParameterFact(-1, "COMPASS_OFS3_Y")
    property Fact compass3OfsZFact: controller.getParameterFact(-1, "COMPASS_OFS3_Z")

    property bool compass1Available: compass1IdFact.value !== 0
    property bool compass2Available: compass2IdFact.value !== 0
    property bool compass3Available: compass3IdFact.value !== 0

    property bool compass1Calibrated: compass1Available ? compass1OfsXFact.value != 0.0  && compass1OfsYFact.value != 0.0  &&compass1OfsZFact.value != 0.0 : false
    property bool compass2Calibrated: compass2Available ? compass2OfsXFact.value != 0.0  && compass2OfsYFact.value != 0.0  &&compass2OfsZFact.value != 0.0 : false
    property bool compass3Calibrated: compass3Available ? compass3OfsXFact.value != 0.0  && compass3OfsYFact.value != 0.0  &&compass3OfsZFact.value != 0.0 : false

    property bool compassCalNeeded: controller.compassSetupNeeded

    Column {
        anchors.fill:       parent
        anchors.margins:    8

        VehicleSummaryRow {
            labelText: qsTr("Compass 1:")
            visible:    compass1Available
            valueText:  compass1Calibrated ? qsTr("Ready") : qsTr("Setup required")
        }

        VehicleSummaryRow {
            labelText: qsTr("Compass 2:")
            visible:    compass2Available
            valueText:  compass2Calibrated ? qsTr("Ready") : qsTr("Setup required")
        }

        VehicleSummaryRow {
            labelText: qsTr("Compass 3:")
            visible:    compass3Available
            valueText:  compass3Calibrated ? qsTr("Ready") : qsTr("Setup required")
        }

        VehicleSummaryRow {
            labelText: qsTr("Accelerometer:")
            valueText: controller.accelSetupNeeded ? qsTr("Setup required") : qsTr("Ready")
        }
    }
}

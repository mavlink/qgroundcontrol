import QtQuick 2.2
import QtQuick.Controls 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Controllers 1.0
import QGroundControl.Palette 1.0

FactPanel {
    id:                 panel
    anchors.fill:       parent
    color:              qgcPal.windowShadeDark

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }
    APMVideoStreamingComponentController { id: controller; factPanel: panel}

    property bool isStart: controller.videoStatusFact.value

    Column {
        anchors.fill:       parent
        VehicleSummaryRow {
            labelText: qsTr("State:")
            valueText: isStart ? qsTr("Running") : qsTr("Stopped")
        }
        VehicleSummaryRow {
            labelText: qsTr("Target IP:")
            valueText: controller.targetIp
        }

        VehicleSummaryRow {
            labelText: qsTr("Target port:")
            valueText: controller.targetPort
        }
    }
}

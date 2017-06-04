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
    VideoStreamingComponentController { id: controller; factPanel: panel}

    property int number: controller.cameras.length

    Column {
        anchors.fill:       parent
        VehicleSummaryRow {
            labelText: qsTr("Available cameras: ")
            valueText: number
        }
        Repeater {
            id:    info
            model: controller.cameraNames

            VehicleSummaryRow {
                labelText: qsTr("Name:")
                valueText: modelData
            }
        }
    }
}

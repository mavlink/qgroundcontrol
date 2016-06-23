import QtQuick          2.5
import QtQuick.Controls 1.2

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0

FactPanel {
    id:             panel
    anchors.fill:   parent
    color:          qgcPal.windowShadeDark

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }
    FactPanelController { id: controller; factPanel: panel }

    property var _vehicle: controller.vehicle

    property Fact flightMode1: controller.getParameterFact(-1, _vehicle.rover ? "MODE1" : "FLTMODE1")
    property Fact flightMode2: controller.getParameterFact(-1, _vehicle.rover ? "MODE2" : "FLTMODE2")
    property Fact flightMode3: controller.getParameterFact(-1, _vehicle.rover ? "MODE3" : "FLTMODE3")
    property Fact flightMode4: controller.getParameterFact(-1, _vehicle.rover ? "MODE4" : "FLTMODE4")
    property Fact flightMode5: controller.getParameterFact(-1, _vehicle.rover ? "MODE5" : "FLTMODE5")
    property Fact flightMode6: controller.getParameterFact(-1, _vehicle.rover ? "MODE6" : "FLTMODE6")

    Column {
        anchors.fill:       parent

        VehicleSummaryRow {
            labelText: qsTr("Flight Mode 1:")
            valueText: flightMode1.enumStringValue
        }

        VehicleSummaryRow {
            labelText: qsTr("Flight Mode 2:")
            valueText: flightMode2.enumStringValue
        }

        VehicleSummaryRow {
            labelText: qsTr("Flight Mode 3:")
            valueText: flightMode3.enumStringValue
        }

        VehicleSummaryRow {
            labelText: qsTr("Flight Mode 4:")
            valueText: flightMode4.enumStringValue
        }

        VehicleSummaryRow {
            labelText: qsTr("Flight Mode 5:")
            valueText: flightMode5.enumStringValue
        }

        VehicleSummaryRow {
            labelText: qsTr("Flight Mode 6:")
            valueText: flightMode6.enumStringValue
        }
    }
}

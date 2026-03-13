import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

Item {
    implicitWidth: mainLayout.implicitWidth
    implicitHeight: mainLayout.implicitHeight
    width: parent.width  // grows when Loader is wider than implicitWidth

    property string _naString: qsTr("N/A")

    FactPanelController { id: controller; }

    BatteryParams {
        id:             battParams
        controller:     controller
        batteryIndex:   1
    }

    ColumnLayout {
        id: mainLayout
        spacing: 0

        VehicleSummaryRow {
            labelText: qsTr("Battery Source")
            valueText: battParams.battSource.enumStringValue
        }

        VehicleSummaryRow {
            labelText: qsTr("Battery Full")
            valueText: battParams.battHighVoltAvailable ? battParams.battHighVolt.valueString + " " + battParams.battHighVolt.units : _naString
        }

        VehicleSummaryRow {
            labelText: qsTr("Battery Empty")
            valueText: battParams.battLowVoltAvailable ? battParams.battLowVolt.valueString + " " + battParams.battLowVolt.units : _naString
        }

        VehicleSummaryRow {
            labelText: qsTr("Number of Cells")
            valueText: battParams.battNumCellsAvailable ? battParams.battNumCells.valueString : _naString
        }
    }
}

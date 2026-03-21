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

    property int _indexedBatteryParamCount: {
        var batteryIndex = 1
        while (controller.parameterExists(-1, "BAT" + batteryIndex + "_SOURCE")) {
            batteryIndex++
        }
        return batteryIndex - 1
    }

    ColumnLayout {
        id: mainLayout
        spacing: 0

        Repeater {
            model: _indexedBatteryParamCount

            Loader {
                sourceComponent: batterySummaryComponent

                property int    batteryIndex:       index + 1
                property bool   showBatteryIndex:   _indexedBatteryParamCount > 1
            }
        }
    }

    Component {
        id: batterySummaryComponent

        ColumnLayout {
            spacing: 0

            property var  _controller:      controller
            property int  _batteryIndex:    batteryIndex

            BatteryParams {
                id:             battParams
                controller:     _controller
                batteryIndex:   _batteryIndex
            }

            VehicleSummaryRow {
                labelText: showBatteryIndex ? qsTr("Battery %1 Source").arg(batteryIndex) : qsTr("Battery Source")
                valueText: battParams.battSource.enumStringValue
            }

            VehicleSummaryRow {
                labelText: showBatteryIndex ? qsTr("Battery %1 Full").arg(batteryIndex) : qsTr("Battery Full")
                valueText: battParams.battHighVoltAvailable ? battParams.battHighVolt.valueString + " " + battParams.battHighVolt.units : _naString
            }

            VehicleSummaryRow {
                labelText: showBatteryIndex ? qsTr("Battery %1 Empty").arg(batteryIndex) : qsTr("Battery Empty")
                valueText: battParams.battLowVoltAvailable ? battParams.battLowVolt.valueString + " " + battParams.battLowVolt.units : _naString
            }

            VehicleSummaryRow {
                labelText: showBatteryIndex ? qsTr("Battery %1 Number of Cells").arg(batteryIndex) : qsTr("Number of Cells")
                valueText: battParams.battNumCellsAvailable ? battParams.battNumCells.valueString : _naString
            }
        }
    }
}

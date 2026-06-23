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

    FactPanelController { id: controller }

    APMBatteryParams {
        id:             battParams
        controller:     controller
        batteryIndex:   0
    }

    ColumnLayout {
        id: mainLayout
        spacing: 0

        Repeater {
            model: battParams.getBatteryCount()

            delegate: ColumnLayout {
                required property int index
                spacing: 0
                visible: _monitorEnabled

                property string _prefix:            battParams.prefixForIndex(index)
                property string _label:             battParams.labelForIndex(index)
                property Fact   _monitor:           controller.getParameterFact(-1, _prefix + "MONITOR")
                property bool   _monitorEnabled:    _monitor.rawValue !== 0
                property bool   _capacityAvailable: controller.parameterExists(-1, _prefix + "CAPACITY")
                property Fact   _capacity:          _capacityAvailable ? controller.getParameterFact(-1, _prefix + "CAPACITY") : null

                VehicleSummaryRow {
                    labelText: qsTr("Batt%1 monitor").arg(_label)
                    valueText: _monitor.enumStringValue
                }

                VehicleSummaryRow {
                    labelText: qsTr("Batt%1 capacity").arg(_label)
                    valueText: _capacity ? _capacity.valueString + " " + _capacity.units : ""
                    visible:   _capacityAvailable
                }
            }
        }
    }
}

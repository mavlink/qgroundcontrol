/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @brief Battery, propeller and magnetometer summary
///     @author Gus Grubba <gus@auterion.com>

import QtQuick
import QtQuick.Controls

import QGroundControl

import QGroundControl.FactControls
import QGroundControl.Controls


Item {
    anchors.fill:   parent

    property string _naString: qsTr("N/A")

    FactPanelController { id: controller; }

    BatteryParams {
        id:             battParams
        controller:     controller
        batteryIndex:   1
    }

    Column {
        anchors.fill:       parent

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

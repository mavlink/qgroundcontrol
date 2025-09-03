/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

Item {
    anchors.fill:   parent

    property var vehicle: globals.activeVehicle

    Column {
        anchors.fill:       parent

        VehicleSummaryRow {
            labelText: qsTr("ESCs Detected")
            valueText: vehicle && vehicle.am32eeproms ? vehicle.am32eeproms.count : qsTr("None")
        }

        VehicleSummaryRow {
            labelText: qsTr("AM32 Support")
            valueText: {
                if (vehicle && vehicle.am32eeproms && vehicle.am32eeproms.count > 0) {
                    return qsTr("Available")
                }
                return qsTr("Unavailable")
            }
        }

        VehicleSummaryRow {
            labelText: qsTr("Status")
            valueText: vehicle && vehicle.am32eeproms && vehicle.am32eeproms.count > 0 ? qsTr("Ready") : qsTr("Not Connected")
        }
    }
}

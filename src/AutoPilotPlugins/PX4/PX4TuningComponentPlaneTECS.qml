/****************************************************************************
 *
 * (c) 2021 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0

ColumnLayout {
    width: availableWidth
    anchors.fill: parent

    PIDTuning {
        width: availableWidth
        property var data: QtObject {
            property string name: qsTr("Altitude & Airspeed")
            property var plot: [
                { name: "Airspeed", value: globals.activeVehicle.airSpeed.value },
                { name: "Airspeed Setpoint", value: globals.activeVehicle.airSpeedSetpoint.value },
                { name: "Altitide (Rel)", value: globals.activeVehicle.altitudeTuning.value },
                { name: "Altitude Setpoint", value: globals.activeVehicle.altitudeTuningSetpoint.value }
            ]
            property var params: ListModel {
                ListElement {
                    title:          qsTr("Height rate feed forward (FW_T_HRATE_FF)")
                    description:    qsTr("TODO")
                    param:          "FW_T_HRATE_FF"
                    min:            0
                    max:            1
                    step:           0.05
                }
            }
            // TODO: add other params
        }
        title: "TECS"
        tuningMode: Vehicle.ModeAltitudeAndAirspeed
        unit: ""
        axis: [ data ]
    }
}




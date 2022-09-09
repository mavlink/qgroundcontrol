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
    property alias autotuningEnabled: pidTuning.autotuningEnabled

    PIDTuning {
        width: availableWidth
        id:    pidTuning

        property var roll: QtObject {
            property string name: qsTr("Roll")
            property var plot: [
                { name: "Response", value: globals.activeVehicle.roll.value },
                { name: "Setpoint", value: globals.activeVehicle.setpoint.roll.value }
            ]
            property var params: ListModel {
                ListElement {
                    title:          qsTr("Time constant (FW_R_TC)")
                    description:    qsTr("The latency between a roll step input and the achieved setpoint (inverse to a P gain)")
                    param:          "FW_R_TC"
                    min:            0.4
                    max:            1.0
                    step:           0.05
                }
            }
        }
        property var pitch: QtObject {
            property string name: qsTr("Pitch")
            property var plot: [
                { name: "Response", value: globals.activeVehicle.pitch.value },
                { name: "Setpoint", value: globals.activeVehicle.setpoint.pitch.value }
            ]
            property var params: ListModel {
                ListElement {
                    title:          qsTr("Time Constant (FW_P_TC)")
                    description:    qsTr("The latency between a pitch step input and the achieved setpoint (inverse to a P gain)")
                    param:          "FW_P_TC"
                    min:            0.2
                    max:            1.0
                    step:           0.05
                }
            }
        }
        title: "Attitude"
        tuningMode: Vehicle.ModeRateAndAttitude
        unit: "deg"
        axis: [ roll, pitch ]
        showAutoModeChange: true
        showAutoTuning:     true
    }
}

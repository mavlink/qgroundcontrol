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

        title: "Attitude"
        tuningMode: Vehicle.ModeRateAndAttitude
        unit: "deg"
        axis: [ yaw ]
        showAutoModeChange: true
        showAutoTuning:     true

        property var yaw: QtObject {
            property string name: qsTr("Yaw")
            
            property var plot: [
                { name: "Response", value: globals.activeVehicle.yaw.value },
                { name: "Setpoint", value: globals.activeVehicle.setpoint.yaw.value }
            ]

            property var params: ListModel {
                ListElement {
                    title:          qsTr("Attitude P-gain (GND_ATT_P)")
                    description:    qsTr("P-gain for setting rate setpoint based on attitude error")
                    param:          "GND_ATT_P"
                    min:            0.0
                    max:            1.0
                    step:           0.05
                }
            }
        }
        
    }
}

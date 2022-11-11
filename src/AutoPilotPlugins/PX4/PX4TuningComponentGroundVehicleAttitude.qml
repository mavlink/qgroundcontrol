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
        id:    pidTuning

        title: "Attitude"
        tuningMode: Vehicle.ModeRateAndAttitude
        unit: "deg"
        axis: [ yaw ]

        property var yaw: QtObject {
            property string name: qsTr("Yaw")
            
            property var plot: [
                { name: "Response", value: globals.activeVehicle.heading.value },
                { name: "Setpoint", value: globals.activeVehicle.setpoint.yaw.value }
            ]

            property var params: ListModel {
//                Note: Having this `GND_ATT_P` parameter's unit as 'rad' in PX4 messed up the QGC's Fact system, and the sliders were
//                considered as `deg` (by default, not sure why that's happening). So the P-gain getting
//                written was multiplied by PI / 180. This is likely a QGC Bug.
                ListElement {
                    title:          qsTr("Attitude P-gain (GND_ATT_P)")
                    description:    qsTr("P-gain for setting rate setpoint based on attitude error")
                    param:          "GND_ATT_P"
                    min:            0.0
                    max:            5.0
                    step:           0.05
                }
                ListElement {
                    title:          qsTr("Maximum Manual Attitude Control Yaw rate (GND_MAN_Y_MAX) [deg/s]")
                    description:    qsTr("In Attitude mode, maximum roll stick deflection will command yaw rate of `GND_MAN_Y_MAX`")
                    param:          "GND_MAN_Y_MAX"
                    min:            0.0
                    max:            400.0
                    step:           5.0
                }
            }
        }
        
    }
}

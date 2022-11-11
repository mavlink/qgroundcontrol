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

        title: "Rate"
        tuningMode: Vehicle.ModeRateAndAttitude
        unit: "deg/s"
        axis: [ yaw ]
        chartDisplaySec: 3

        // Yaw tuning parameters
        property var yaw: QtObject {
            property string name: qsTr("Yaw")

            property var plot: [
                { name: "Response", value: globals.activeVehicle.yawRate.value },
                { name: "Setpoint", value: globals.activeVehicle.setpoint.yawRate.value }
            ]

            property var params: ListModel {
                ListElement {
                    title:          qsTr("Maximum Yaw rate (GND_RATE_MAX) [rad/s]")
                    description:    qsTr("Maximum Yaw rate reachable by the vehicle's actuator & body. If in Manual mode, edit `GND_MAN_Y_MAX`")
                    param:          "GND_RATE_MAX"
                    min:            0.0
                    max:            5.0
                    step:           0.05
                    // SITL Boat can reach upto ~17 deg/s == 0.25 rad/s | Real Racing Boat can reach upto 6.00 rad/s
                }
                ListElement {
                    title:          qsTr("Feedforward Gain (GND_RATE_FF)")
                    description:    qsTr("Feedforward used to compensate for gronds damping. Cruicial for Boats")
                    param:          "GND_RATE_FF"
                    min:            0.0
                    max:            10.0
                    step:           0.05
                }
                ListElement {
                    title:          qsTr("Porportional Gain (GND_RATE_P)")
                    description:    qsTr("Porportional Gain.")
                    param:          "GND_RATE_P"
                    min:            0.0
                    max:            5.0
                    step:           0.005
                }
                ListElement {
                    title:          qsTr("Integral Gain (GND_RATE_I)")
                    description:    qsTr("Generally does not need much adjustment, reduce this when seeing slow oscillations.")
                    param:          "GND_RATE_I"
                    min:            0.0
                    max:            1.0
                    step:           0.005
                }
                ListElement {
                    title:          qsTr("Derivative Gain (GND_RATE_D)")
                    description:    qsTr("Derivative Gain")
                    param:          "GND_RATE_D"
                    min:            0.0
                    max:            1.0
                    step:           0.005
                }
                ListElement {
                    title:          qsTr("Integral max (GND_RATE_IMAX)")
                    description:    qsTr("Sanity limit to prevent integrator from running away.")
                    param:          "GND_RATE_IMAX"
                    min:            0.0
                    max:            50.0
                    step:           0.5
                }
            }
        }
    }
}


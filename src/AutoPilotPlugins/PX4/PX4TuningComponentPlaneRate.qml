/****************************************************************************
 *
 * (c) 2021 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactSystem
import QGroundControl.FactControls
import QGroundControl.ScreenTools
import QGroundControl.Vehicle

ColumnLayout {
    property real _availableHeight: availableHeight
    property real _availableWidth:  availableWidth

    PIDTuning {
        id:                 pidTuning
        availableWidth:     _availableWidth
        availableHeight:    _availableHeight - pidTuning.y

        property var roll: QtObject {
            property string name: qsTr("Roll")
            property var plot: [
                { name: "Response", value: globals.activeVehicle.rollRate.value },
                { name: "Setpoint", value: globals.activeVehicle.setpoint.rollRate.value }
            ]
            property var params: ListModel {
                ListElement {
                    title:          qsTr("Porportional gain (FW_RR_P)")
                    description:    qsTr("Porportional gain.")
                    param:          "FW_RR_P"
                    min:            0.0
                    max:            1
                    step:           0.005
                }
                ListElement {
                    title:          qsTr("Differential Gain (FW_RR_D)")
                    description:    qsTr("Damping: increase to reduce overshoots and oscillations, but not higher than really needed.")
                    param:          "FW_RR_D"
                    min:            0.0
                    max:            1.0
                    step:           0.005
                }
                ListElement {
                    title:          qsTr("Integral Gain (FW_RR_I)")
                    description:    qsTr("Generally does not need much adjustment, reduce this when seeing slow oscillations.")
                    param:          "FW_RR_I"
                    min:            0.0
                    max:            0.5
                    step:           0.005
                }
                ListElement {
                    title:          qsTr("Feedforward Gain (FW_RR_FF)")
                    description:    qsTr("Feedforward gused to compensate for aerodynamic damping.")
                    param:          "FW_RR_FF"
                    min:            0.0
                    max:            10.0
                    step:           0.05
                }
            }
        }
        property var pitch: QtObject {
            property string name: qsTr("Pitch")
            property var plot: [
                { name: "Response", value: globals.activeVehicle.pitchRate.value },
                { name: "Setpoint", value: globals.activeVehicle.setpoint.pitchRate.value }
            ]
            property var params: ListModel {
                ListElement {
                    title:          qsTr("Porportional Gain (FW_PR_P)")
                    description:    qsTr("Porportional Gain.")
                    param:          "FW_PR_P"
                    min:            0.0
                    max:            1
                    step:           0.005
                }
                ListElement {
                    title:          qsTr("Differential Gain (FW_PR_D)")
                    description:    qsTr("Damping: increase to reduce overshoots and oscillations, but not higher than really needed.")
                    param:          "FW_PR_D"
                    min:            0.0
                    max:            1.00
                    step:           0.005
                }
                ListElement {
                    title:          qsTr("Integral Gain (FW_PR_I)")
                    description:    qsTr("Generally does not need much adjustment, reduce this when seeing slow oscillations.")
                    param:          "FW_PR_I"
                    min:            0.0
                    max:            0.5
                    step:           0.005
                }
                ListElement {
                    title:          qsTr("Feedforward Gain (FW_PR_FF)")
                    description:    qsTr("Feedforward gused to compensate for aerodynamic damping.")
                    param:          "FW_PR_FF"
                    min:            0.0
                    max:            10.0
                    step:           0.05
                }
            }
        }
        property var yaw: QtObject {
            property string name: qsTr("Yaw")
            property var plot: [
                { name: "Response", value: globals.activeVehicle.yawRate.value },
                { name: "Setpoint", value: globals.activeVehicle.setpoint.yawRate.value }
            ]
            property var params: ListModel {
                ListElement {
                    title:          qsTr("Porportional Gain (FW_YR_P)")
                    description:    qsTr("Porportional Gain.")
                    param:          "FW_YR_P"
                    min:            0.0
                    max:            1
                    step:           0.005
                }
                ListElement {
                    title:          qsTr("Integral Gain (FW_YR_D)")
                    description:    qsTr("Generally does not need much adjustment, reduce this when seeing slow oscillations.")
                    param:          "FW_YR_D"
                    min:            0.0
                    max:            1.0
                    step:           0.005
                }
                ListElement {
                    title:          qsTr("Integral Gain (FW_YR_I)")
                    description:    qsTr("Generally does not need much adjustment, reduce this when seeing slow oscillations.")
                    param:          "FW_YR_I"
                    min:            0.0
                    max:            50.0
                    step:           0.5
                }
                ListElement {
                    title:          qsTr("Feedforward Gain (FW_YR_FF)")
                    description:    qsTr("Feedforward gused to compensate for aerodynamic damping.")
                    param:          "FW_YR_FF"
                    min:            0.0
                    max:            10.0
                    step:           0.05
                }
                ListElement {
                    title:          qsTr("Roll control to yaw feedforward (FW_RLL_TO_YAW_FF)")
                    description:    qsTr("Used to counteract the adverse yaw effect for fixed wings.")
                    param:          "FW_RLL_TO_YAW_FF"
                    min:            0.0
                    max:            1.0
                    step:           0.01
                }
            }
        }
        title: "Rate"
        tuningMode: Vehicle.ModeRateAndAttitude
        unit: "deg/s"
        axis: [ roll, pitch, yaw ]
        chartDisplaySec: 3
        showAutoModeChange: true
        showAutoTuning:     true
    }
}


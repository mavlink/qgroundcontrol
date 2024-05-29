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
    property real _availableHeight:     availableHeight
    property real _availableWidth:      availableWidth
    property Fact _airmode:             controller.getParameterFact(-1, "MC_AIRMODE", false)
    property Fact _thrustModelFactor:   controller.getParameterFact(-1, "THR_MDL_FAC", false)

    PIDTuning {
        id:                 pidTuning
        availableWidth:     _availableWidth
        availableHeight:    _availableHeight - pidTuning.y
        title:              qsTr("Rate")
        tuningMode:         Vehicle.ModeRateAndAttitude
        unit:               qsTr("deg/s")
        axis:               [ roll, pitch, yaw ]
        chartDisplaySec:    3
        showAutoModeChange: false
        showAutoTuning:     false

        property var roll: QtObject {
            property string name: qsTr("Roll")
            property var plot: [
                { name: "Response", value: globals.activeVehicle.rollRate.value },
                { name: "Setpoint", value: globals.activeVehicle.setpoint.rollRate.value }
            ]
            property var params: ListModel {
                ListElement {
                    title:          qsTr("Overall Multiplier (SC_ROLLRATE_K)")
                    description:    qsTr("Multiplier for P, I and D gains: increase for more responsiveness, reduce if the rates overshoot (and increasing D does not help).")
                    param:          "SC_ROLLRATE_K"
                    min:            0.0
                    max:            10.0
                    step:           0.005
                }
                ListElement {
                    title:          qsTr("Differential Gain (SC_ROLLRATE_D)")
                    description:    qsTr("Damping: increase to reduce overshoots and oscillations, but not higher than really needed.")
                    param:          "SC_ROLLRATE_D"
                    min:            0.0000
                    max:            10.0
                    step:           0.005
                }
                ListElement {
                    title:          qsTr("Integral Gain (SC_ROLLRATE_I)")
                    description:    qsTr("Generally does not need much adjustment, reduce this when seeing slow oscillations.")
                    param:          "SC_ROLLRATE_I"
                    min:            0.0
                    max:            10.0
                    step:           0.005
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
                    title:          qsTr("Overall Multiplier (SC_PITCHRATE_K)")
                    description:    qsTr("Multiplier for P, I and D gains: increase for more responsiveness, reduce if the rates overshoot (and increasing D does not help).")
                    param:          "SC_PITCHRATE_K"
                    min:            0.0
                    max:            10.0
                    step:           0.005
                }
                ListElement {
                    title:          qsTr("Differential Gain (SC_PITCHRATE_D)")
                    description:    qsTr("Damping: increase to reduce overshoots and oscillations, but not higher than really needed.")
                    param:          "SC_PITCHRATE_D"
                    min:            0.0
                    max:            10.0
                    step:           0.005
                }
                ListElement {
                    title:          qsTr("Integral Gain (SC_PITCHRATE_I)")
                    description:    qsTr("Generally does not need much adjustment, reduce this when seeing slow oscillations.")
                    param:          "SC_PITCHRATE_I"
                    min:            0.0
                    max:            10.0
                    step:           0.005
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
                    title:          qsTr("Overall Multiplier (SC_YAWRATE_K)")
                    description:    qsTr("Multiplier for P, I and D gains: increase for more responsiveness, reduce if the rates overshoot (and increasing D does not help).")
                    param:          "SC_YAWRATE_K"
                    min:            0.0
                    max:            10.0
                    step:           0.005
                }
                ListElement {
                    title:          qsTr("Integral Gain (SC_YAWRATE_D)")
                    description:    qsTr("Generally does not need much adjustment, reduce this when seeing slow oscillations.")
                    param:          "SC_YAWRATE_D"
                    min:            0.0
                    max:            10.0
                    step:           0.005
                }
                ListElement {
                    title:          qsTr("Integral Gain (SC_YAWRATE_I)")
                    description:    qsTr("Generally does not need much adjustment, reduce this when seeing slow oscillations.")
                    param:          "SC_YAWRATE_I"
                    min:            0.0
                    max:            10.0
                    step:           0.005
                }
                ListElement {
                    title:          qsTr("Integral Limit (SC_YR_INT_LIM)")
                    description:    qsTr("Adjust if integral error is present (robot keeps spinning with 0 setpoint).")
                    param:          "SC_YR_INT_LIM"
                    min:            0.0
                    max:            5.0
                    step:           0.005
                }
            }
        }
    }
}


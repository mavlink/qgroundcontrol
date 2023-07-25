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
    property Fact _airmode:           controller.getParameterFact(-1, "MC_AIRMODE", false)
    property Fact _thrustModelFactor: controller.getParameterFact(-1, "THR_MDL_FAC", false)
    property alias autotuningEnabled: pidTuning.autotuningEnabled

    GridLayout {
        columns: 2

        QGCLabel {
            textFormat:         Text.RichText
            text:               qsTr("Airmode (disable during tuning) <b><a href=\"https://docs.px4.io/master/en/config_mc/pid_tuning_guide_multicopter.html#airmode-mixer-saturation\">?</a></b>:")
            onLinkActivated:    Qt.openUrlExternally(link)
            visible:            _airmode
        }
        FactComboBox {
            fact:               _airmode
            indexModel:         false
            visible:            _airmode
        }

        QGCLabel {
            textFormat:         Text.RichText
            text:               qsTr("Thrust curve <b><a href=\"https://docs.px4.io/master/en/config_mc/pid_tuning_guide_multicopter.html#thrust-curve\">?</a></b>:")
            onLinkActivated:    Qt.openUrlExternally(link)
            visible:            _thrustModelFactor
        }
        FactTextField {
            fact:               _thrustModelFactor
            visible:            _thrustModelFactor
        }
    }
    PIDTuning {
        width: availableWidth
        id:    pidTuning

        property var roll: QtObject {
            property string name: qsTr("Roll")
            property var plot: [
                { name: "Response", value: globals.activeVehicle.rollRate.value },
                { name: "Setpoint", value: globals.activeVehicle.setpoint.rollRate.value }
            ]
            property var params: ListModel {
                ListElement {
                    title:          qsTr("Overall Multiplier (MC_ROLLRATE_K)")
                    description:    qsTr("Multiplier for P, I and D gains: increase for more responsiveness, reduce if the rates overshoot (and increasing D does not help).")
                    param:          "MC_ROLLRATE_K"
                    min:            0.3
                    max:            3
                    step:           0.05
                }
                ListElement {
                    title:          qsTr("Differential Gain (MC_ROLLRATE_D)")
                    description:    qsTr("Damping: increase to reduce overshoots and oscillations, but not higher than really needed.")
                    param:          "MC_ROLLRATE_D"
                    min:            0.0004
                    max:            0.01
                    step:           0.0002
                }
                ListElement {
                    title:          qsTr("Integral Gain (MC_ROLLRATE_I)")
                    description:    qsTr("Generally does not need much adjustment, reduce this when seeing slow oscillations.")
                    param:          "MC_ROLLRATE_I"
                    min:            0.1
                    max:            0.5
                    step:           0.025
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
                    title:          qsTr("Overall Multiplier (MC_PITCHRATE_K)")
                    description:    qsTr("Multiplier for P, I and D gains: increase for more responsiveness, reduce if the rates overshoot (and increasing D does not help).")
                    param:          "MC_PITCHRATE_K"
                    min:            0.3
                    max:            3
                    step:           0.05
                }
                ListElement {
                    title:          qsTr("Differential Gain (MC_PITCHRATE_D)")
                    description:    qsTr("Damping: increase to reduce overshoots and oscillations, but not higher than really needed.")
                    param:          "MC_PITCHRATE_D"
                    min:            0.0004
                    max:            0.01
                    step:           0.0002
                }
                ListElement {
                    title:          qsTr("Integral Gain (MC_PITCHRATE_I)")
                    description:    qsTr("Generally does not need much adjustment, reduce this when seeing slow oscillations.")
                    param:          "MC_PITCHRATE_I"
                    min:            0.1
                    max:            0.5
                    step:           0.025
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
                    title:          qsTr("Overall Multiplier (MC_YAWRATE_K)")
                    description:    qsTr("Multiplier for P, I and D gains: increase for more responsiveness, reduce if the rates overshoot (and increasing D does not help).")
                    param:          "MC_YAWRATE_K"
                    min:            0.3
                    max:            3
                    step:           0.05
                }
                ListElement {
                    title:          qsTr("Integral Gain (MC_YAWRATE_I)")
                    description:    qsTr("Generally does not need much adjustment, reduce this when seeing slow oscillations.")
                    param:          "MC_YAWRATE_I"
                    min:            0.04
                    max:            0.4
                    step:           0.02
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


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

    // If set to 1, use closed-loop PID control using GPS velocity. If 0, fix throttle at cruise throttle of mission settings.
    property Fact _gndVehicleSpeedControlMode: controller.getParameterFact(-1, "GND_SP_CTRL_MODE", false)

    // Reminder for setting the velocity control mode to closed-loop PID control
    GridLayout {
        columns: 2

        QGCLabel {
            text:               qsTr("Velocity control mode (set this to 'Close the loop' for PID tuning):")
            visible:            !_gndVehicleSpeedControlMode
        }

        FactComboBox {
            fact:               _gndVehicleSpeedControlMode
            indexModel:         false
        }
    }

    PIDTuning {
        width: availableWidth
        id:    pidTuning

        title: "Velocity"
        tuningMode: Vehicle.ModeVelocityAndPosition
        unit: "m/s"
        axis: [ all ]

        property var all: QtObject {
            property string name: qsTr("Velocity")
            
            property var plot: [
                { name: "Vel X Response", value: globals.activeVehicle.localPosition.vx.value },
                { name: "Vel X Setpoint", value: globals.activeVehicle.localPositionSetpoint.vx.value },
                { name: "Vel Y Response", value: globals.activeVehicle.localPosition.vy.value },
                { name: "Vel Y Setpoint", value: globals.activeVehicle.localPositionSetpoint.vy.value }
            ]

            property var params: ListModel {
                ListElement {
                    title:          qsTr("Trim ground speed (GND_SPEED_TRIM)")
                    description:    qsTr("Trim speed that will be the target speed for missions")
                    param:          "GND_SPEED_TRIM"
                    min:            0.0
                    max:            20.0
                    step:           0.1
                },
                ListElement {
                    title:          qsTr("Minimum ground speed (GND_SPEED_MIN)")
                    description:    qsTr("Minimum ground speed")
                    param:          "GND_SPEED_MIN"
                    min:            0.0
                    max:            20.0
                    step:           0.1
                },
                ListElement {
                    title:          qsTr("Maximum ground speed (GND_SPEED_MAX)")
                    description:    qsTr("")
                    param:          "GND_SPEED_MAX"
                    min:            0.0
                    max:            20.0
                    step:           0.1
                },
                ListElement {
                    title:          qsTr("Speed to throttle scalar (GND_SPEED_THR_SC)")
                    description:    qsTr("This is a gain to map the speed control output to the throttle linearly.")
                    param:          "GND_SPEED_THR_SC"
                    min:            0.0
                    max:            5.0
                    step:           0.1
                },
                ListElement {
                    title:          qsTr("Porportional Gain (GND_SPEED_P)")
                    description:    qsTr("Porportional Gain.")
                    param:          "GND_SPEED_P"
                    min:            0.0
                    max:            10.0
                    step:           0.01
                },
                ListElement {
                    title:          qsTr("Integral Gain (GND_SPEED_I)")
                    description:    qsTr("Generally does not need much adjustment, reduce this when seeing slow oscillations.")
                    param:          "GND_SPEED_I"
                    min:            0.0
                    max:            10.0
                    step:           0.01
                },
                ListElement {
                    title:          qsTr("Derivative Gain (GND_SPEED_D)")
                    description:    qsTr("Derivative gain for velocity controller")
                    param:          "GND_SPEED_D"
                    min:            0.0
                    max:            10.0
                    step:           0.01
                },
                ListElement {
                    title:          qsTr("Speed Integral maximum value (GND_SPEED_IMAX)")
                    description:    qsTr("Maximum value integral can reach to prevent wind-up")
                    param:          "GND_SPEED_IMAX"
                    min:            0.0
                    max:            1.0
                    step:           0.01
                }
            }
        }
        
    }
}

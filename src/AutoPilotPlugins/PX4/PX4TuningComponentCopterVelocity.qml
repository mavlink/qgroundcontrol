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
    property Fact _mcPosMode:       controller.getParameterFact(-1, "MPC_POS_MODE", false)

    GridLayout {
        columns: 2

        QGCLabel {
            text:               qsTr("Position control mode (set this to 'simple' during tuning):")
            visible:            _mcPosMode
        }
        FactComboBox {
            fact:               _mcPosMode
            indexModel:         false
            visible:            _mcPosMode
        }
    }

    PIDTuning {
        width: availableWidth
        property var horizontal: QtObject {
            property string name: qsTr("Horizontal")
            property string plotTitle: qsTr("Horizontal (Y direction, sidewards)")
            property var plot: [
                { name: "Response", value: globals.activeVehicle.localPosition.vy.value },
                { name: "Setpoint", value: globals.activeVehicle.localPositionSetpoint.vy.value }
            ]
            property var params: ListModel {
                ListElement {
                    title:          qsTr("Proportional gain (MPC_XY_VEL_P_ACC)")
                    description:    qsTr("Increase for more responsiveness, reduce if the velocity overshoots (and increasing D does not help).")
                    param:          "MPC_XY_VEL_P_ACC"
                    min:            1.2
                    max:            5
                    step:           0.05
                }
                ListElement {
                    title:          qsTr("Integral gain (MPC_XY_VEL_I_ACC)")
                    description:    qsTr("Increase to reduce steady-state error (e.g. wind)")
                    param:          "MPC_XY_VEL_I_ACC"
                    min:            0.2
                    max:            10
                    step:           0.2
                }
                ListElement {
                    title:          qsTr("Differential gain (MPC_XY_VEL_D_ACC)")
                    description:    qsTr("Damping: increase to reduce overshoots and oscillations, but not higher than really needed.")
                    param:          "MPC_XY_VEL_D_ACC"
                    min:            0.1
                    max:            2
                    step:           0.05
                }
            }
        }
        property var vertical: QtObject {
            property string name: qsTr("Vertical")
            property var plot: [
                { name: "Response", value: globals.activeVehicle.localPosition.vz.value },
                { name: "Setpoint", value: globals.activeVehicle.localPositionSetpoint.vz.value }
            ]
            property var params: ListModel {
                ListElement {
                    title:          qsTr("Proportional gain (MPC_Z_VEL_P_ACC)")
                    description:    qsTr("Increase for more responsiveness, reduce if the velocity overshoots (and increasing D does not help).")
                    param:          "MPC_Z_VEL_P_ACC"
                    min:            2
                    max:            15
                    step:           0.5
                }
                ListElement {
                    title:          qsTr("Integral gain (MPC_Z_VEL_I_ACC)")
                    description:    qsTr("Increase to reduce steady-state error")
                    param:          "MPC_Z_VEL_I_ACC"
                    min:            0.2
                    max:            3
                    step:           0.05
                }
                ListElement {
                    title:          qsTr("Differential gain (MPC_Z_VEL_D_ACC)")
                    description:    qsTr("Damping: increase to reduce overshoots and oscillations, but not higher than really needed.")
                    param:          "MPC_Z_VEL_D_ACC"
                    min:            0
                    max:            2
                    step:           0.05
                }
            }
        }
        title: "Velocity"
        tuningMode: Vehicle.ModeVelocityAndPosition
        unit: "m/s"
        axis: [ horizontal, vertical ]
    }
}



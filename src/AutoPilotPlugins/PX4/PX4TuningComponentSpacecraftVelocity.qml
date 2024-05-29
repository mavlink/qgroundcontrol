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

        property var position: QtObject {
            property string name: qsTr("Velocity")
            property string plotTitle: qsTr("Velocity (X axis)")
            property var plot: [
                { name: "Response", value: globals.activeVehicle.localPosition.vx.value },
                { name: "Setpoint", value: globals.activeVehicle.localPositionSetpoint.vx.value }
            ]
            property var params: ListModel {
                ListElement {
                    title:          qsTr("Proportional gain (SPC_VEL_P)")
                    description:    qsTr("Increase for more responsiveness, reduce if the velocity overshoots (and increasing D does not help).")
                    param:          "SPC_VEL_P"
                    min:            0.0
                    max:            20
                    step:           0.05
                }
                ListElement {
                    title:          qsTr("Differential gain (SPC_VEL_D)")
                    description:    qsTr("Damping: increase to reduce overshoots and oscillations, but not higher than really needed.")
                    param:          "SPC_VEL_D"
                    min:            0.0
                    max:            3
                    step:           0.05
                }
                ListElement {
                    title:          qsTr("Integral gain (SPC_VEL_I)")
                    description:    qsTr("Increase to reduce steady-state error (e.g. wind)")
                    param:          "SPC_VEL_I"
                    min:            0.0
                    max:            10
                    step:           0.2
                }
                ListElement {
                    title:          qsTr("Integral gain Limiter (SPC_VEL_I_LIM)")
                    description:    qsTr("Increase to enlarge the allowed integral compensation.")
                    param:          "SPC_VEL_I_LIM"
                    min:            0.0
                    max:            5
                    step:           0.2
                }
            }
        }
        title: "Velocity"
        tuningMode: Vehicle.ModeVelocityAndPosition
        unit: "m/s"
        axis: [ position]
    }
}



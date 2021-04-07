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
                { name: "Response", value: globals.activeVehicle.localPosition.y.value },
                { name: "Setpoint", value: globals.activeVehicle.localPositionSetpoint.y.value }
            ]
            property var params: ListModel {
                ListElement {
                    title:          qsTr("Proportional gain (MPC_XY_P)")
                    description:    qsTr("Increase for more responsiveness, reduce if the position overshoots (there is only a setpoint when hovering, i.e. when centering the stick).")
                    param:          "MPC_XY_P"
                    min:            0
                    max:            2
                    step:           0.05
                }
            }
        }
        property var vertical: QtObject {
            property string name: qsTr("Vertical")
            property var plot: [
                { name: "Response", value: globals.activeVehicle.localPosition.z.value },
                { name: "Setpoint", value: globals.activeVehicle.localPositionSetpoint.z.value }
            ]
            property var params: ListModel {
                ListElement {
                    title:          qsTr("Proportional gain (MPC_Z_P)")
                    description:    qsTr("Increase for more responsiveness, reduce if the position overshoots (there is only a setpoint when hovering, i.e. when centering the stick).")
                    param:          "MPC_Z_P"
                    min:            0
                    max:            2
                    step:           0.05
                }
            }
        }
        title: "Position"
        tuningMode: Vehicle.ModeVelocityAndPosition
        unit: "m"
        axis: [ horizontal, vertical ]
        chartDisplaySec: 50
    }
}



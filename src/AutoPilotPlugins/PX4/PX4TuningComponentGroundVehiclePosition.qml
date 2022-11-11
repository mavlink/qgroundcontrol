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

        title: "Position"
        tuningMode: Vehicle.ModeVelocityAndPosition
        unit: ""
        axis: [ all ]

        property var all: QtObject {
            property string name: qsTr("Position")

            property var plot: [
                { name: "Pos X Response", value: globals.activeVehicle.localPosition.x.value },
                { name: "Pos X Setpoint", value: globals.activeVehicle.localPositionSetpoint.x.value },
                { name: "Pos Y Response", value: globals.activeVehicle.localPosition.y.value },
                { name: "Pos Y Setpoint", value: globals.activeVehicle.localPositionSetpoint.y.value }
            ]

            property var params: ListModel {
                ListElement {
                    title:          qsTr("L1 Waypoint activation distance (GND_L1_DIST)")
                    description:    qsTr("Distance at which the next waypoint is activated")
                    param:          "GND_L1_DIST"
                    min:            1.0
                    max:            50.0
                    step:           0.1
                }
                ListElement {
                    title:          qsTr("L1 guidance distance (GND_L1_PERIOD)")
                    description:    qsTr("L1 distance definining the tracking point ahead of the ground vehicle it's following")
                    param:          "GND_L1_PERIOD"
                    min:            0.5
                    max:            50.0
                    step:           0.1
                }
                ListElement {
                    title:          qsTr("Cruise throttle (GND_THR_CRUISE)")
                    description:    qsTr("Throttle setting required for achieving desired cruise speed (shouldn't this be automatically be estimated?)")
                    param:          "GND_THR_CRUISE"
                    min:            0.0
                    max:            1.0
                    step:           0.01
                }
                ListElement {
                    title:          qsTr("Minimum throttle (GND_THR_MIN)")
                    description:    qsTr("Minimum throttle outputted by the controller")
                    param:          "GND_THR_MIN"
                    min:            0.0
                    max:            1.0
                    step:           0.01
                }
                ListElement {
                    title:          qsTr("Maximum throttle (GND_THR_MAX)")
                    description:    qsTr("Maximum throttle outputted by the controller")
                    param:          "GND_THR_MAX"
                    min:            0.0
                    max:            1.0
                    step:           0.01
                }
            }
        }
        
    }
}




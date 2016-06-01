/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick              2.5
import QtQuick.Controls     1.4

import QGroundControl.Controls  1.0

FactSliderPanel {
    anchors.fill:   parent
    panelTitle:     "Tuning"

    sliderModel: ListModel {
        ListElement {
            title:          qsTr("Hover Throttle")
            description:    qsTr("Adjust throttle so hover is at mid-throttle. Slide to the left if hover is lower than throttle center. Slide to the right if hover is higher than throttle center.")
            param:          "MPC_THR_HOVER"
            min:            20
            max:            80
            step:           1
        }

        ListElement {
            title:          qsTr("Manual minimum throttle")
            description:    qsTr("Slide to the left to start the motors with less idle power. Slide to the right if descending in manual flight becomes unstable.")
            param:          "MPC_MANTHR_MIN"
            min:            0
            max:            15
            step:           1
        }

        ListElement {
            title:          qsTr("Roll sensitivity")
            description:    qsTr("Slide to the left to make roll control faster and more accurate. Slide to the right if roll oscillates or is too twitchy.")
            param:          "MC_ROLL_TC"
            min:            0.15
            max:            0.25
            step:           0.01
        }

        ListElement {
            title:          qsTr("Pitch sensitivity")
            description:    qsTr("Slide to the left to make pitch control faster and more accurate. Slide to the right if pitch oscillates or is too twitchy.")
            param:          "MC_PITCH_TC"
            min:            0.15
            max:            0.25
            step:           0.01
        }

        ListElement {
            title:          qsTr("Altitude control sensitivity")
            description:    qsTr("Slide to the left to make altitude control smoother and less twitchy. Slide to the right to make altitude control more accurate and more aggressive.")
            param:          "MPC_Z_FF"
            min:            0
            max:            1.0
            step:           0.1
        }

        ListElement {
            title:          qsTr("Position control sensitivity")
            description:    qsTr("Slide to the left to make flight in position control mode smoother and less twitchy. Slide to the right to make position control more accurate and more aggressive.")
            param:          "MPC_XY_FF"
            min:            0
            max:            1.0
            step:           0.1
        }
    }
}

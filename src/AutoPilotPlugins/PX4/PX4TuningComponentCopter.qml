/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

import QtQuick              2.5
import QtQuick.Controls     1.4

import QGroundControl.Controls  1.0

FactSliderPanel {
    anchors.fill:   parent
    panelTitle:     "Tuning"

    sliderModel: ListModel {
        ListElement {
            title:          qsTr("Throttle Hover")
            description:    qsTr("Adjust throttle so hover is at mid-throttle. Slide to the left if hover is lower than throttle center. Slide to the right if hover is higher than throttle center.")
            param:          "MPC_THR_HOVER"
            min:            0.2
            max:            0.8
            step:           0.01
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

        ListElement {
            title:          qsTr("Manual minimum throttle")
            description:    qsTr("Slide to the left to start the motors with less idle power. Slide to the right if descending in manual flight becomes unstable.")
            param:          "MPC_MANTHR_MIN"
            min:            0
            max:            0.15
            step:           0.01
        }
    }
}

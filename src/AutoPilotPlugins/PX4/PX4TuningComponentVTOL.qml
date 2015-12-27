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
    anchors.fill: parent

    sliderModel: ListModel {
        ListElement {
            title:          "Roll sensitivity"
            description:    "Slide to the left to make roll control faster and more accurate. Slide to the right if roll oscillates or is too twitchy."
            param:          "FW_R_TC"
            min:            0.2
            max:            0.8
            step:           0.01
        }

        ListElement {
            title:          "Pitch sensitivity"
            description:    "Slide to the left to make pitch control faster and more accurate. Slide to the right if pitch oscillates or is too twitchy."
            param:          "FW_P_TC"
            min:            0.2
            max:            0.8
            step:           0.01
        }

        ListElement {
            title:          "Cruise throttle"
            description:    "This is the throttle setting required to achieve the desired cruise speed. Most planes need 50-60%."
            param:          "FW_THR_CRUISE"
            min:            0.2
            max:            0.8
            step:           0.01
        }

        ListElement {
            title:          "Mission mode sensitivity"
            description:    "Slide to the left to make position control more accurate and more aggressive. Slide to the right to make flight in mission mode smoother and less twitchy."
            param:          "FW_L1_PERIOD"
            min:            12
            max:            50
            step:           0.5
        }
    }
}

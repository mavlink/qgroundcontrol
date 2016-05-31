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
            min:            20
            max:            80
            step:           1
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

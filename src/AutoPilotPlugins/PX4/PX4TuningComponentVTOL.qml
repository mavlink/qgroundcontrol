/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick              2.3
import QtQuick.Controls     1.2

import QGroundControl.Controls  1.0

SetupPage {
    id:             tuningPage
    pageComponent:  pageComponent

    Component {
        id: pageComponent

        FactSliderPanel {
            width:          availableWidth
            sliderModel: ListModel {

                ListElement {
                    title:          qsTr("Plane Roll sensitivity")
                    description:    qsTr("Slide to the left to make roll control faster and more accurate. Slide to the right if roll oscillates or is too twitchy.")
                    param:          "FW_R_TC"
                    min:            0.2
                    max:            0.8
                    step:           0.01
                }

                ListElement {
                    title:          qsTr("Plane Pitch sensitivity")
                    description:    qsTr("Slide to the left to make pitch control faster and more accurate. Slide to the right if pitch oscillates or is too twitchy.")
                    param:          "FW_P_TC"
                    min:            0.2
                    max:            0.8
                    step:           0.01
                }

                ListElement {
                    title:          qsTr("Plane Cruise throttle")
                    description:    qsTr("This is the throttle setting required to achieve the desired cruise speed. Most planes need 50-60%.")
                    param:          "FW_THR_CRUISE"
                    min:            20
                    max:            80
                    step:           1
                }

                ListElement {
                    title:          qsTr("Hover Throttle")
                    description:    qsTr("Adjust throttle so hover is at mid-throttle. Slide to the left if hover is lower than throttle center. Slide to the right if hover is higher than throttle center.")
                    param:          "MPC_THR_HOVER"
                    min:            20
                    max:            80
                    step:           1
                }

                ListElement {
                    title:          qsTr("Hover manual minimum throttle")
                    description:    qsTr("Slide to the left to start the motors with less idle power. Slide to the right if descending in manual flight becomes unstable.")
                    param:          "MPC_MANTHR_MIN"
                    min:            0
                    max:            15
                    step:           1
                }

                ListElement {
                    title:          qsTr("Plane Mission mode sensitivity")
                    description:    qsTr("Slide to the left to make position control more accurate and more aggressive. Slide to the right to make flight in mission mode smoother and less twitchy.")
                    param:          "FW_L1_PERIOD"
                    min:            12
                    max:            50
                    step:           0.5
                }
            }
        }
    }
}

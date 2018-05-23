/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
            qgcViewPanel:   tuningPage.viewPanel

            sliderModel: ListModel {
                ListElement {
                    title:          qsTr("Roll sensitivity")
                    description:    qsTr("FW_R_TC: Slide to the left to make roll control faster and more accurate. Slide to the right if roll oscillates or is too twitchy.")
                    param:          "FW_R_TC"
                    min:            0.2
                    max:            0.8
                    step:           0.01
                }

                ListElement {
                    title:          qsTr("Pitch sensitivity")
                    description:    qsTr("FW_P_TC: Slide to the left to make pitch control faster and more accurate. Slide to the right if pitch oscillates or is too twitchy.")
                    param:          "FW_P_TC"
                    min:            0.2
                    max:            0.8
                    step:           0.01
                }

                ListElement {
                    title:          qsTr("Cruise throttle")
                    description:    qsTr("FW_THR_CRUISE: This is the throttle setting required to achieve the desired cruise speed. Most planes need 50-60%.")
                    param:          "FW_THR_CRUISE"
                    min:            20
                    max:            80
                    step:           1
                }

                ListElement {
                    title:          qsTr("Mission mode sensitivity")
                    description:    qsTr("FW_L1_PERIOD: Slide to the left to make position control more accurate and more aggressive. Slide to the right to make flight in mission mode smoother and less twitchy.")
                    param:          "FW_L1_PERIOD"
                    min:            12
                    max:            50
                    step:           0.5
                }

                ListElement {
                    title:          qsTr("FW_PR_P")
                    description:    qsTr("Pitch rate proportional gain.")
                    param:          "FW_PR_P"
                    min:            0.005
                    max:            1
                    step:           0.005
                }

                ListElement {
                    title:          qsTr("FW_PR_I")
                    description:    qsTr("Pitch rate integrator gain.")
                    param:          "FW_PR_I"
                    min:            0.005
                    max:            0.5
                    step:           0.005
                }

                ListElement {
                    title:          qsTr("FW_RR_P")
                    description:    qsTr("Roll rate proportional gain.")
                    param:          "FW_RR_P"
                    min:            0.005
                    max:            1
                    step:           0.005
                }

                ListElement {
                    title:          qsTr("FW_RR_I")
                    description:    qsTr("Roll rate integrator gain.")
                    param:          "FW_RR_I"
                    min:            0.005
                    max:            0.2
                    step:           0.005
                }

                ListElement {
                    title:          qsTr("FW_YR_P")
                    description:    qsTr("Yaw rate proportional gain.")
                    param:          "FW_YR_P"
                    min:            0.005
                    max:            1
                    step:           0.005
                }

                ListElement {
                    title:          qsTr("FW_YR_I")
                    description:    qsTr("Yaw rate integrator gain.")
                    param:          "FW_YR_I"
                    min:            0
                    max:            50
                    step:           0.5
                }

                ListElement {
                    title:          qsTr("FW_RR_FF")
                    description:    qsTr("Roll rate feed forward.")
                    param:          "FW_RR_FF"
                    min:            0
                    max:            10
                    step:           0.05
                }

                ListElement {
                    title:          qsTr("FW_PR_FF")
                    description:    qsTr("Pitch rate feed forward.")
                    param:          "FW_PR_FF"
                    min:            0
                    max:            10
                    step:           0.05
                }

                ListElement {
                    title:          qsTr("FW_YR_FF")
                    description:    qsTr("Yaw rate feed forward.")
                    param:          "FW_YR_FF"
                    min:            0
                    max:            10
                    step:           0.05
                }

                ListElement {
                    title:          qsTr("FW_PSP_OFF")
                    description:    qsTr("Pitch setpoint offset.")
                    param:          "FW_PSP_OFF"
                    min:            -90
                    max:            90
                    step:           0.5
                }

            }
        }
    }
}

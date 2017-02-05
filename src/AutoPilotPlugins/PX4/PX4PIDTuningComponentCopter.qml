/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick 2.5
import QtQuick.Controls 1.4

import QGroundControl.Controls 1.0

SetupPage {
    id: tuningPage
    pageComponent: pageComponent

    Component {
        id: pageComponent

        FactStepperPanel {
            id:             factStepperPannel
            width:          availableWidth
            height:         contentHeight
            qgcViewPanel:   tuningPage.viewPanel

            steppersModel: ListModel {

                ///////////////////////////////////////////////////////////////
                ListElement {
                    param: "MC_ROLL_P"
                    group: "roll"
                    stabilized: true
                }

                ListElement {
                    param: "MC_ROLLRATE_P"
                    group: "roll"
                }

                ListElement {
                    param: "MC_ROLLRATE_I"
                    group: "roll"
                }

                ListElement {
                    param: "MC_ROLLRATE_D"
                    group: "roll"
                }

                ListElement {
                    param: "MC_ROLL_TC"
                    group: "roll"
                    advanced: true
                }
                ListElement {
                    param: "MC_ROLLRATE_FF"
                    group: "roll"
                    advanced: true
                }

                ///////////////////////////////////////////////////////////////
                ListElement {
                    param: "MC_PITCH_P"
                    group: "pitch"
                    stabilized: true
                }

                ListElement {
                    param: "MC_PITCHRATE_P"
                    group: "pitch"
                }

                ListElement {
                    param: "MC_PITCHRATE_I"
                    group: "pitch"
                }

                ListElement {
                    param: "MC_PITCHRATE_D"
                    group: "pitch"
                }

                ListElement {
                    param: "MC_PITCH_TC"
                    group: "pitch"
                    advanced: true
                }

                ListElement {
                    param: "MC_PITCHRATE_FF"
                    group: "pitch"
                    advanced: true
                }

                ///////////////////////////////////////////////////////////////
                ListElement {
                    param: "MC_YAW_P"
                    group: "yaw"
                    stabilized: true
                }

                ListElement {
                    param: "MC_YAW_FF"
                    group: "yaw"
                }

                ListElement {
                    param: "MC_YAWRATE_P"
                    group: "yaw"
                }

                ListElement {
                    param: "MC_YAWRATE_I"
                    group: "yaw"
                }

                ListElement {
                    param: "MC_YAWRATE_D"
                    group: "yaw"
                    advanced: true
                }

                ListElement {
                    param: "MC_YAWRATE_FF"
                    group: "yaw"
                    advanced: true
                }

                ///////////////////////////////////////////////////////////////
                ListElement {
                    param: "MC_TPA_BREAK_P"
                    group: "tpa"
                    advanced: false
                }
                ListElement {
                    param: "MC_TPA_BREAK_I"
                    group: "tpa"
                    advanced: true
                }
                ListElement {
                    param: "MC_TPA_BREAK_D"
                    group: "tpa"
                    advanced: true
                }
                ListElement {
                    param: "MC_TPA_RATE_P"
                    group: "tpa"
                    advanced: false
                }
                ListElement {
                    param: "MC_TPA_RATE_I"
                    group: "tpa"
                    advanced: true
                }
                ListElement {
                    param: "MC_TPA_RATE_D"
                    group: "tpa"
                    advanced: true
                }
            }
        }
    }
}

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
                    subGroup: "p"
                    stabilized: true
                }

                ListElement {
                    param: "MC_ROLLRATE_P"
                    group: "roll"
                    subGroup: "p"
                }

                ListElement {
                    param: "MC_ROLLRATE_I"
                    group: "roll"
                    subGroup: "i"
                }

                ListElement {
                    param: "MC_ROLLRATE_D"
                    group: "roll"
                    subGroup: "d"
                }

                ListElement {
                    param: "MC_ROLL_TC"
                    group: "roll"
                    subGroup: "tc"
                    advanced: true
                }
                ListElement {
                    param: "MC_ROLLRATE_FF"
                    group: "roll"
                    subGroup: "ff"
                    advanced: true
                }

                ///////////////////////////////////////////////////////////////
                ListElement {
                    param: "MC_PITCH_P"
                    group: "pitch"
                    subGroup: "p"
                    stabilized: true
                }

                ListElement {
                    param: "MC_PITCHRATE_P"
                    group: "pitch"
                    subGroup: "p"
                }

                ListElement {
                    param: "MC_PITCHRATE_I"
                    group: "pitch"
                    subGroup: "i"
                }

                ListElement {
                    param: "MC_PITCHRATE_D"
                    group: "pitch"
                    subGroup: "d"
                }

                ListElement {
                    param: "MC_PITCH_TC"
                    group: "pitch"
                    subGroup: "tc"
                    advanced: true
                }

                ListElement {
                    param: "MC_PITCHRATE_FF"
                    group: "pitch"
                    subGroup: "ff"
                    advanced: true
                }

                ///////////////////////////////////////////////////////////////
                ListElement {
                    param: "MC_YAW_P"
                    group: "yaw"
                    subGroup: "p"
                    stabilized: true
                }

                ListElement {
                    param: "MC_YAW_FF"
                    group: "yaw"
                    subGroup: "ff"
                }

                ListElement {
                    param: "MC_YAWRATE_P"
                    group: "yaw"
                    subGroup: "p"
                }

                ListElement {
                    param: "MC_YAWRATE_I"
                    group: "yaw"
                    subGroup: "i"
                }

                ListElement {
                    param: "MC_YAWRATE_D"
                    group: "yaw"
                    subGroup: "d"
                    advanced: true
                }

                ListElement {
                    param: "MC_YAWRATE_FF"
                    group: "yaw"
                    subGroup: "ff"
                    advanced: true
                }

                ///////////////////////////////////////////////////////////////
                ListElement {
                    param: "MC_TPA_BREAK_P"
                    group: "tpa"
                    subGroup: "p"
                    advanced: false
                }
                ListElement {
                    param: "MC_TPA_BREAK_I"
                    group: "tpa"
                    subGroup: "i"
                    advanced: true
                }
                ListElement {
                    param: "MC_TPA_BREAK_D"
                    group: "tpa"
                    subGroup: "d"
                    advanced: true
                }
                ListElement {
                    param: "MC_TPA_RATE_P"
                    group: "tpa"
                    subGroup: "p"
                    advanced: false
                }
                ListElement {
                    param: "MC_TPA_RATE_I"
                    group: "tpa"
                    subGroup: "i"
                    advanced: true
                }
                ListElement {
                    param: "MC_TPA_RATE_D"
                    group: "tpa"
                    subGroup: "d"
                    advanced: true
                }
            }
        }
    }
}

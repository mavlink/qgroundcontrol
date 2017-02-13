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
                    group: qsTr("Roll")
                    acro: false
                }

                ListElement {
                    param: "MC_ROLLRATE_P"
                    group: qsTr("Roll")
                }

                ListElement {
                    param: "MC_ROLLRATE_I"
                    group: qsTr("Roll")
                }

                ListElement {
                    param: "MC_ROLLRATE_D"
                    group: qsTr("Roll")
                }

                ListElement {
                    param: "MC_ROLL_TC"
                    group: qsTr("Roll")
                    advanced: true
                }
                ListElement {
                    param: "MC_ROLLRATE_FF"
                    group: qsTr("Roll")
                    advanced: true
                }

                ///////////////////////////////////////////////////////////////
                ListElement {
                    param: "MC_PITCH_P"
                    group: qsTr("Pitch")
                    acro: false
                }

                ListElement {
                    param: "MC_PITCHRATE_P"
                    group: qsTr("Pitch")
                }

                ListElement {
                    param: "MC_PITCHRATE_I"
                    group: qsTr("Pitch")
                }

                ListElement {
                    param: "MC_PITCHRATE_D"
                    group: qsTr("Pitch")
                }

                ListElement {
                    param: "MC_PITCH_TC"
                    group: qsTr("Pitch")
                    advanced: true
                }

                ListElement {
                    param: "MC_PITCHRATE_FF"
                    group: qsTr("Pitch")
                    advanced: true
                }

                ///////////////////////////////////////////////////////////////
                ListElement {
                    param: "MC_YAW_P"
                    group: qsTr("Yaw")
                    acro: false
                }

                ListElement {
                    param: "MC_YAW_FF"
                    group: qsTr("Yaw")
                    acro: false
                }

                ListElement {
                    param: "MC_YAWRATE_P"
                    group: qsTr("Yaw")
                }

                ListElement {
                    param: "MC_YAWRATE_I"
                    group: qsTr("Yaw")
                }

                ListElement {
                    param: "MC_YAWRATE_D"
                    group: qsTr("Yaw")
                    advanced: true
                }

                ListElement {
                    param: "MC_YAWRATE_FF"
                    group: qsTr("Yaw")
                    advanced: true
                }

                ///////////////////////////////////////////////////////////////
                ListElement {
                    param: "MC_TPA_BREAK_P"
                    group: qsTr("Throttle PID Attenuation (TPA)")
                }
                ListElement {
                    param: "MC_TPA_BREAK_I"
                    group: qsTr("Throttle PID Attenuation (TPA)")
                }
                ListElement {
                    param: "MC_TPA_BREAK_D"
                    group: qsTr("Throttle PID Attenuation (TPA)")
                }
                ListElement {
                    param: "MC_TPA_RATE_P"
                    group: qsTr("Throttle PID Attenuation (TPA)")
                }
                ListElement {
                    param: "MC_TPA_RATE_I"
                    group: qsTr("Throttle PID Attenuation (TPA)")
                }
                ListElement {
                    param: "MC_TPA_RATE_D"
                    group: qsTr("Throttle PID Attenuation (TPA)")
                }
            }
        }
    }
}

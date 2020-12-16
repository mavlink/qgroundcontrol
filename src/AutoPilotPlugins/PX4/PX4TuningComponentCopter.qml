/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 1.2
import QtCharts         2.2
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.ScreenTools   1.0

SetupPage {
    id:             tuningPage
    pageComponent:  pageComponent

    Component {
        id: pageComponent

        Column {
            width: availableWidth

            Component.onCompleted: {
                // We use QtCharts only on Desktop platforms
                showAdvanced = !ScreenTools.isMobile
            }

            FactPanelController {
                id:         controller
            }

            // Standard tuning page
            FactSliderPanel {
                width:          availableWidth
                visible:        !advanced

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
                }
            }

            Loader {
                anchors.left:       parent.left
                anchors.right:      parent.right
                sourceComponent:    advanced ? advancePageComponent : undefined
            }

            Component {
                id: advancePageComponent


                PIDTuning {
                    property var rollList: ListModel {
                        ListElement {
                            title:          qsTr("Overall Multiplier (MC_ROLLRATE_K)")
                            description:    qsTr("Multiplier for P, I and D gains: increase for more responsiveness, reduce if the rates overshoot (and increasing D does not help).")
                            param:          "MC_ROLLRATE_K"
                            min:            0.3
                            max:            3
                            step:           0.05
                        }
                        ListElement {
                            title:          qsTr("Differential Gain (MC_ROLLRATE_D)")
                            description:    qsTr("Damping: increase to reduce overshoots and oscillations, but not higher than really needed.")
                            param:          "MC_ROLLRATE_D"
                            min:            0.0004
                            max:            0.01
                            step:           0.0002
                        }
                        ListElement {
                            title:          qsTr("Integral Gain (MC_ROLLRATE_I)")
                            description:    qsTr("Generally does not need much adjustment, reduce this when seeing slow oscillations.")
                            param:          "MC_ROLLRATE_I"
                            min:            0.1
                            max:            0.5
                            step:           0.025
                        }
                    }
                    property var pitchList: ListModel {
                        ListElement {
                            title:          qsTr("Overall Multiplier (MC_PITCHRATE_K)")
                            description:    qsTr("Multiplier for P, I and D gains: increase for more responsiveness, reduce if the rates overshoot (and increasing D does not help).")
                            param:          "MC_PITCHRATE_K"
                            min:            0.3
                            max:            3
                            step:           0.05
                        }
                        ListElement {
                            title:          qsTr("Differential Gain (MC_PITCHRATE_D)")
                            description:    qsTr("Damping: increase to reduce overshoots and oscillations, but not higher than really needed.")
                            param:          "MC_PITCHRATE_D"
                            min:            0.0004
                            max:            0.01
                            step:           0.0002
                        }
                        ListElement {
                            title:          qsTr("Integral Gain (MC_PITCHRATE_I)")
                            description:    qsTr("Generally does not need much adjustment, reduce this when seeing slow oscillations.")
                            param:          "MC_PITCHRATE_I"
                            min:            0.1
                            max:            0.5
                            step:           0.025
                        }
                    }
                    property var yawList: ListModel {
                        ListElement {
                            title:          qsTr("Overall Multiplier (MC_YAWRATE_K)")
                            description:    qsTr("Multiplier for P, I and D gains: increase for more responsiveness, reduce if the rates overshoot (and increasing D does not help).")
                            param:          "MC_YAWRATE_K"
                            min:            0.3
                            max:            3
                            step:           0.05
                        }
                        ListElement {
                            title:          qsTr("Integral Gain (MC_YAWRATE_I)")
                            description:    qsTr("Generally does not need much adjustment, reduce this when seeing slow oscillations.")
                            param:          "MC_YAWRATE_I"
                            min:            0.04
                            max:            0.4
                            step:           0.02
                        }
                    }
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    tuneList:            [ qsTr("Roll"), qsTr("Pitch"), qsTr("Yaw") ]
                    params:              [
                        rollList,
                        pitchList,
                        yawList
                    ]
                }
            } // Component - Advanced Page
        } // Column
    } // Component - pageComponent
} // SetupPage

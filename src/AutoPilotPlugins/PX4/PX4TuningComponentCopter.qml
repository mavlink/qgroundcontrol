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

            FactPanelController {
                id:         controller
            }

            PIDTuning {
                width: availableWidth
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
        } // Column
    } // Component - pageComponent
} // SetupPage

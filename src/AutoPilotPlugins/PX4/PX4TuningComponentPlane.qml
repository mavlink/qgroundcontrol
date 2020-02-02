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
                        title:          qsTr("Cruise throttle")
                        description:    qsTr("This is the throttle setting required to achieve the desired cruise speed. Most planes need 50-60%.")
                        param:          "FW_THR_CRUISE"
                        min:            20
                        max:            80
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
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    tuneList:            [ qsTr("Roll"), qsTr("Pitch"), qsTr("Yaw") ]
                    params:              [
                        [ controller.getParameterFact(-1, "FW_RR_P"),
                         controller.getParameterFact(-1, "FW_RR_I"),
                         controller.getParameterFact(-1, "FW_RR_FF"),
                         controller.getParameterFact(-1, "FW_R_TC"),],
                        [ controller.getParameterFact(-1, "FW_PR_P"),
                         controller.getParameterFact(-1, "FW_PR_I"),
                         controller.getParameterFact(-1, "FW_PR_FF"),
                         controller.getParameterFact(-1, "FW_P_TC") ],
                        [ controller.getParameterFact(-1, "FW_YR_P"),
                         controller.getParameterFact(-1, "FW_YR_I"),
                         controller.getParameterFact(-1, "FW_YR_FF") ] ]
                }
            } // Component - Advanced Page
        } // Column
    } // Component - pageComponent
} // SetupPage

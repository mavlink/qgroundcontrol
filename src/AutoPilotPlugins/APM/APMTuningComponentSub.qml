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

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

SetupPage {
    id:             tuningPage
    pageComponent:  tuningPageComponent

    Component {
        id: tuningPageComponent

        Column {
            width:      availableWidth
            spacing:    _margins

            FactPanelController { id: controller; }

            QGCPalette { id: palette; colorGroupEnabled: true }

            property real _margins: ScreenTools.defaultFontPixelHeight

            ExclusiveGroup { id: buttonGroup }

            Row {
                spacing: _margins
                QGCButton {
                    id:             atcButton
                    text:           qsTr("Attitude Controller Parameters")
                    exclusiveGroup: buttonGroup
                    checked:        true
                    onClicked:      checked = true
                }

                QGCButton {
                    id:             posButton
                    text:           qsTr("Position Controller Parameters")
                    exclusiveGroup: buttonGroup
                    onClicked:      checked = true
                }

                QGCButton {
                    id:             navButton
                    text:           qsTr("Waypoint navigation parameters")
                    exclusiveGroup: buttonGroup
                    onClicked:      checked = true
                }
            }

            Rectangle {
                id:                 atcParams
                visible:            atcButton.checked
                anchors.left:       parent.left
                anchors.right:      parent.right
                height:             posColumn.height + _margins*2
                color:              palette.windowShade

                Column {
                    id:                 posColumn
                    width:              parent.width/2
                    anchors.margins:    _margins
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.top:        parent.top
                    spacing:            _margins*1.5

                    FactTextFieldSlider { fact: controller.getParameterFact(-1, "ATC_ANG_PIT_P") }
                    FactTextFieldSlider { fact: controller.getParameterFact(-1, "ATC_ANG_RLL_P") }
                    FactTextFieldSlider { fact: controller.getParameterFact(-1, "ATC_ANG_YAW_P") }
                    FactTextFieldSlider { fact: controller.getParameterFact(-1, "ATC_RAT_PIT_P") }
                    FactTextFieldSlider { fact: controller.getParameterFact(-1, "ATC_RAT_PIT_I") }
                    FactTextFieldSlider { fact: controller.getParameterFact(-1, "ATC_RAT_PIT_IMAX") }
                    FactTextFieldSlider { fact: controller.getParameterFact(-1, "ATC_RAT_PIT_D") }
                    FactTextFieldSlider { fact: controller.getParameterFact(-1, "ATC_RAT_RLL_P") }
                    FactTextFieldSlider { fact: controller.getParameterFact(-1, "ATC_RAT_RLL_I") }
                    FactTextFieldSlider { fact: controller.getParameterFact(-1, "ATC_RAT_RLL_IMAX") }
                    FactTextFieldSlider { fact: controller.getParameterFact(-1, "ATC_RAT_RLL_D") }
                    FactTextFieldSlider { fact: controller.getParameterFact(-1, "ATC_RAT_YAW_P") }
                    FactTextFieldSlider { fact: controller.getParameterFact(-1, "ATC_RAT_YAW_I") }
                    FactTextFieldSlider { fact: controller.getParameterFact(-1, "ATC_RAT_YAW_IMAX") }
                    FactTextFieldSlider { fact: controller.getParameterFact(-1, "ATC_RAT_YAW_D") }

                } // Column - Position Controller Parameters
            } // Rectangle - Position Controller Parameters

            Rectangle {
                id:                 posParams
                visible:            posButton.checked
                anchors.left:       parent.left
                anchors.right:      parent.right
                height:             velColumn.height + _margins*2
                color:              palette.windowShade

                Component {
                    id: velColumnUpTo36
                    Column {
                        anchors.margins:    _margins
                        anchors.left:       parent.left
                        anchors.right:      parent.right
                        anchors.top:        parent.top
                        spacing:            _margins*1.5

                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "r.PSC_POSXY_P") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "r.PSC_POSZ_P") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "r.PSC_VELXY_P") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "r.PSC_VELXY_I") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "r.PSC_VELXY_IMAX") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "r.PSC_VELZ_P") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "r.PSC_ACCZ_D") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "r.PSC_ACCZ_FILT") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "r.PSC_ACCZ_I") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "r.PSC_ACCZ_IMAX") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "r.PSC_ACCZ_P") }

                    } // Column - VEL parameters
                }
                Component {
                    id: velColumn40
                    Column {
                        anchors.margins:    _margins
                        anchors.left:       parent.left
                        anchors.right:      parent.right
                        anchors.top:        parent.top
                        spacing:            _margins*1.5

                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "r.PSC_POSXY_P") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "r.PSC_POSZ_P") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "r.PSC_VELXY_P") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "r.PSC_VELXY_I") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "r.PSC_VELXY_IMAX") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "r.PSC_VELZ_P") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "r.PSC_ACCZ_D") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "r.PSC_ACCZ_FLTD") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "r.PSC_ACCZ_FLTE") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "r.PSC_ACCZ_FLTT") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "r.PSC_ACCZ_I") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "r.PSC_ACCZ_IMAX") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "r.PSC_ACCZ_P") }

                    } // Column - VEL parameters
                }
                Loader {
                    id:                 velColumn
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.top:        parent.top

                    sourceComponent: activeVehicle.versionCompare(3, 6, 0) <= 0 ? velColumnUpTo36 :velColumn40
                }
            } // Rectangle - VEL parameters

            Rectangle {
                id:                 navParams
                visible:            navButton.checked
                anchors.left:       parent.left
                anchors.right:      parent.right
                height:             wpnavColumn.height + _margins*2
                color:              palette.windowShade

                // WPNAV parameters up to 3.5
                Component {
                    id: wpnavColumn35
                    Column {
                        anchors.margins:    _margins
                        anchors.left:       parent.left
                        anchors.right:      parent.right
                        anchors.top:        parent.top
                        spacing:            _margins*1.5


                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "WPNAV_ACCEL") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "WPNAV_ACCEL_Z") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "WPNAV_LOIT_JERK") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "WPNAV_LOIT_MAXA") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "WPNAV_LOIT_MINA") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "WPNAV_LOIT_SPEED") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "WPNAV_RADIUS") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "WPNAV_SPEED") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "WPNAV_SPEED_DN") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "WPNAV_SPEED_UP") }
                    }
                }

                // WPNAV parameters for 3.6 and upwards
                Component {
                    id: wpnavColumn36
                    Column {
                        anchors.margins:    _margins
                        anchors.left:       parent.left
                        anchors.right:      parent.right
                        anchors.top:        parent.top
                        spacing:            _margins*1.5

                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "WPNAV_ACCEL") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "WPNAV_ACCEL_Z") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "WPNAV_RADIUS") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "WPNAV_SPEED") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "WPNAV_SPEED_DN") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "WPNAV_SPEED_UP") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "LOIT_SPEED") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "LOIT_ACC_MAX") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "LOIT_ANG_MAX") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "LOIT_BRK_ACCEL") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "LOIT_BRK_DELAY") }
                        FactTextFieldSlider { fact: controller.getParameterFact(-1, "LOIT_BRK_JERK") }
                    }
                }

                Loader {
                    id:                 wpnavColumn
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.top:        parent.top

                    sourceComponent: activeVehicle.versionCompare(3, 6, 0) < 0 ? wpnavColumn35 : wpnavColumn36
                    }
            } // Rectangle - WPNAV parameters
        } // Column
    } // Component
} // SetupView

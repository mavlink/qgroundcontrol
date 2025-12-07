/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick
import QtQuick.Controls

import QGroundControl

import QGroundControl.FactControls

import QGroundControl.Controls


SetupPage {
    id:             tuningPage
    pageComponent:  tuningPageComponent

    Component {
        id: tuningPageComponent

        Column {
            width:      availableWidth
            spacing:    _margins

            FactPanelController { id: controller; }

            QGCPalette { id: qgcPal; colorGroupEnabled: true }

            property real _margins: ScreenTools.defaultFontPixelHeight

            Row {
                spacing: _margins
                QGCButton {
                    id:             atcButton
                    text:           qsTr("Attitude Controller Parameters")
                    autoExclusive:  true
                    checked:        true
                    onClicked:      checked = true
                }

                QGCButton {
                    id:             posButton
                    text:           qsTr("Position Controller Parameters")
                    autoExclusive:  true
                    onClicked:      checked = true
                }

                QGCButton {
                    id:             navButton
                    text:           qsTr("Waypoint navigation parameters")
                    autoExclusive:  true
                    onClicked:      checked = true
                }
            }

            Rectangle {
                id:                 atcParams
                visible:            atcButton.checked
                anchors.left:       parent.left
                anchors.right:      parent.right
                height:             posColumn.height + _margins*2
                color:              qgcPal.windowShade

                Column {
                    id:                 posColumn
                    width:              parent.width/2
                    anchors.margins:    _margins
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.top:        parent.top
                    spacing:            _margins*1.5

                    FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "ATC_ANG_PIT_P") }
                    FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "ATC_ANG_RLL_P") }
                    FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "ATC_ANG_YAW_P") }
                    FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "ATC_RAT_PIT_P") }
                    FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "ATC_RAT_PIT_I") }
                    FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "ATC_RAT_PIT_IMAX") }
                    FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "ATC_RAT_PIT_D") }
                    FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "ATC_RAT_RLL_P") }
                    FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "ATC_RAT_RLL_I") }
                    FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "ATC_RAT_RLL_IMAX") }
                    FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "ATC_RAT_RLL_D") }
                    FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "ATC_RAT_YAW_P") }
                    FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "ATC_RAT_YAW_I") }
                    FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "ATC_RAT_YAW_IMAX") }
                    FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "ATC_RAT_YAW_D") }

                } // Column - Position Controller Parameters
            } // Rectangle - Position Controller Parameters

            Rectangle {
                id:                 posParams
                visible:            posButton.checked
                anchors.left:       parent.left
                anchors.right:      parent.right
                height:             velColumn.height + _margins*2
                color:              qgcPal.windowShade

                Component {
                    id: velColumnUpTo36
                    Column {
                        anchors.margins:    _margins
                        anchors.left:       parent.left
                        anchors.right:      parent.right
                        anchors.top:        parent.top
                        spacing:            _margins*1.5

                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "r.PSC_POSXY_P") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "r.PSC_POSZ_P") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "r.PSC_VELXY_P") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "r.PSC_VELXY_I") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "r.PSC_VELXY_IMAX") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "r.PSC_VELZ_P") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "r.PSC_ACCZ_D") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "r.PSC_ACCZ_FILT") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "r.PSC_ACCZ_I") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "r.PSC_ACCZ_IMAX") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "r.PSC_ACCZ_P") }

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

                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "r.PSC_POSXY_P") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "r.PSC_POSZ_P") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "r.PSC_VELXY_P") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "r.PSC_VELXY_I") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "r.PSC_VELXY_IMAX") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "r.PSC_VELZ_P") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "r.PSC_ACCZ_D") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "r.PSC_ACCZ_FLTD") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "r.PSC_ACCZ_FLTE") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "r.PSC_ACCZ_FLTT") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "r.PSC_ACCZ_I") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "r.PSC_ACCZ_IMAX") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "r.PSC_ACCZ_P") }

                    } // Column - VEL parameters
                }
                Loader {
                    id:                 velColumn
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.top:        parent.top

                    sourceComponent: globals.activeVehicle.versionCompare(3, 6, 0) <= 0 ? velColumnUpTo36 :velColumn40
                }
            } // Rectangle - VEL parameters

            Rectangle {
                id:                 navParams
                visible:            navButton.checked
                anchors.left:       parent.left
                anchors.right:      parent.right
                height:             wpnavColumn.height + _margins*2
                color:              qgcPal.windowShade

                // WPNAV parameters up to 3.5
                Component {
                    id: wpnavColumn35
                    Column {
                        anchors.margins:    _margins
                        anchors.left:       parent.left
                        anchors.right:      parent.right
                        anchors.top:        parent.top
                        spacing:            _margins*1.5


                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "WPNAV_ACCEL") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "WPNAV_ACCEL_Z") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "WPNAV_LOIT_JERK") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "WPNAV_LOIT_MAXA") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "WPNAV_LOIT_MINA") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "WPNAV_LOIT_SPEED") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "WPNAV_RADIUS") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "WPNAV_SPEED") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "WPNAV_SPEED_DN") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "WPNAV_SPEED_UP") }
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

                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "WPNAV_ACCEL") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "WPNAV_ACCEL_Z") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "WPNAV_RADIUS") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "WPNAV_SPEED") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "WPNAV_SPEED_DN") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "WPNAV_SPEED_UP") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "LOIT_SPEED") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "LOIT_ACC_MAX") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "LOIT_ANG_MAX") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "LOIT_BRK_ACCEL") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "LOIT_BRK_DELAY") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "LOIT_BRK_JERK") }
                    }
                }

                Loader {
                    id:                 wpnavColumn
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.top:        parent.top

                    sourceComponent: globals.activeVehicle.versionCompare(3, 6, 0) < 0 ? wpnavColumn35 : wpnavColumn36
                    }
            } // Rectangle - WPNAV parameters
        } // Column
    } // Component
} // SetupView

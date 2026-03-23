import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

SetupPage {
    id:             tuningPage
    pageComponent:  tuningPageComponent

    Component {
        id: tuningPageComponent

        ColumnLayout {
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

            QGCGroupBox {
                id:                 atcParams
                Layout.preferredWidth: tuningPage.availableWidth * 0.75
                visible:            atcButton.checked
                title:              qsTr("Attitude Controller Parameters")

                Column {
                    id:                 posColumn
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

                } // Column - Attitude Controller Parameters
            } // QGCGroupBox - Attitude Controller Parameters

            QGCGroupBox {
                id:                 posParams
                Layout.preferredWidth: tuningPage.availableWidth * 0.75
                visible:            posButton.checked
                title:              qsTr("Position Controller Parameters")

                Component {
                    id: velColumnUpTo36
                    Column {
                        anchors.margins:    _margins
                        anchors.left:       parent.left
                        anchors.right:      parent.right
                        anchors.top:        parent.top
                        spacing:            _margins*1.5

                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "PSC_POSXY_P") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "PSC_POSZ_P") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "PSC_VELXY_P") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "PSC_VELXY_I") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "PSC_VELXY_IMAX") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "PSC_VELZ_P") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "PSC_ACCZ_D") }
                        FactTextFieldSlider2 {
                            visible: controller.parameterExists(-1, "PSC_ACCZ_FILT")
                            fact:    visible ? controller.getParameterFact(-1, "PSC_ACCZ_FILT") : null
                        }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "PSC_ACCZ_I") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "PSC_ACCZ_IMAX") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "PSC_ACCZ_P") }

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

                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "PSC_NE_POS_P") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "PSC_D_POS_P") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "PSC_NE_VEL_P") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "PSC_NE_VEL_I") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "PSC_NE_VEL_IMAX") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "PSC_D_VEL_P") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "PSC_D_ACC_D") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "PSC_D_ACC_FLTD") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "PSC_D_ACC_FLTE") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "PSC_D_ACC_FLTT") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "PSC_D_ACC_I") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "PSC_D_ACC_IMAX") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "PSC_D_ACC_P") }

                    } // Column - VEL parameters
                }
                Loader {
                    id:                 velColumn
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.top:        parent.top

                    sourceComponent: globals.activeVehicle.versionCompare(3, 6, 0) <= 0 ? velColumnUpTo36 :velColumn40
                }
            } // QGCGroupBox - Position Controller Parameters

            QGCGroupBox {
                id:                 navParams
                Layout.preferredWidth: tuningPage.availableWidth * 0.75
                visible:            navButton.checked
                title:              qsTr("Waypoint Navigation Parameters")

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

                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "WP_ACC") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "WP_ACC_Z") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "WP_RADIUS_M") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "WP_SPD") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "WP_SPD_DN") }
                        FactTextFieldSlider2 { fact: controller.getParameterFact(-1, "WP_SPD_UP") }
                        FactTextFieldSlider2 {
                            visible: controller.parameterExists(-1, "LOIT_SPEED")
                            fact:    visible ? controller.getParameterFact(-1, "LOIT_SPEED") : null
                        }
                        FactTextFieldSlider2 {
                            visible: controller.parameterExists(-1, "LOIT_ACC_MAX")
                            fact:    visible ? controller.getParameterFact(-1, "LOIT_ACC_MAX") : null
                        }
                        FactTextFieldSlider2 {
                            visible: controller.parameterExists(-1, "LOIT_ANG_MAX")
                            fact:    visible ? controller.getParameterFact(-1, "LOIT_ANG_MAX") : null
                        }
                        FactTextFieldSlider2 {
                            visible: controller.parameterExists(-1, "LOIT_BRK_ACCEL")
                            fact:    visible ? controller.getParameterFact(-1, "LOIT_BRK_ACCEL") : null
                        }
                        FactTextFieldSlider2 {
                            visible: controller.parameterExists(-1, "LOIT_BRK_DELAY")
                            fact:    visible ? controller.getParameterFact(-1, "LOIT_BRK_DELAY") : null
                        }
                        FactTextFieldSlider2 {
                            visible: controller.parameterExists(-1, "LOIT_BRK_JERK")
                            fact:    visible ? controller.getParameterFact(-1, "LOIT_BRK_JERK") : null
                        }
                    }
                }

                Loader {
                    id:                 wpnavColumn
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.top:        parent.top

                    sourceComponent: globals.activeVehicle.versionCompare(3, 6, 0) < 0 ? wpnavColumn35 : wpnavColumn36
                    }
            } // QGCGroupBox - WPNAV parameters
        } // ColumnLayout
    } // Component
} // SetupPage

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

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

SetupPage {
    id: tuningPage
    pageComponent: tuningPageComponent

    Component {
        id: tuningPageComponent

        Column {
            width: availableWidth
            spacing: _margins
            FactPanelController { id: controller; factPanel: tuningPage.viewPanel }

            QGCPalette { id: palette; colorGroupEnabled: true }

            property real _margins: ScreenTools.defaultFontPixelHeight

            Row {
                spacing: _margins
                QGCButton {
                    id: atcButton
                    text: qsTr("Attitude Controller Parameters")
                    altColor: palette.colorBlue

                    onClicked: {
                        atcParams.visible = true
                        posParams.visible = false
                        navParams.visible = false

                        atcButton.altColor = palette.colorBlue
                        posButton.altColor = null
                        navButton.altColor = null
                    }
                }

                QGCButton {
                    id: posButton
                    text: qsTr("Position Controller Parameters")
                    onClicked: {
                        atcParams.visible = false
                        posParams.visible = true
                        navParams.visible = false

                        atcButton.altColor = null
                        posButton.altColor = palette.colorBlue
                        navButton.altColor = null
                    }
                }

                QGCButton {
                    id: navButton
                    text: qsTr("Waypoint navigation parameters")
                    onClicked: {
                        atcParams.visible = false
                        posParams.visible = false
                        navParams.visible = true

                        atcButton.altColor = null
                        posButton.altColor = null
                        navButton.altColor = palette.colorBlue
                    }
                }
            }

            Rectangle {
                id: atcParams
                visible: true
                anchors.left:       parent.left
                anchors.right:      parent.right
                height:             posColumn.height + _margins*2
                color:              palette.windowShade

                Column {
                    id: posColumn
                    width: parent.width/2
                    anchors.margins:    _margins
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.top:        parent.top
                    spacing:            _margins*1.5

                    FactSlider { _fact: controller.getParameterFact(-1, "ATC_ANG_PIT_P") }
                    FactSlider { _fact: controller.getParameterFact(-1, "ATC_ANG_RLL_P") }
                    FactSlider { _fact: controller.getParameterFact(-1, "ATC_ANG_YAW_P") }
                    FactSlider { _fact: controller.getParameterFact(-1, "ATC_RAT_PIT_P") }
                    FactSlider { _fact: controller.getParameterFact(-1, "ATC_RAT_PIT_I") }
                    FactSlider { _fact: controller.getParameterFact(-1, "ATC_RAT_PIT_IMAX") }
                    FactSlider { _fact: controller.getParameterFact(-1, "ATC_RAT_PIT_D") }
                    FactSlider { _fact: controller.getParameterFact(-1, "ATC_RAT_RLL_P") }
                    FactSlider { _fact: controller.getParameterFact(-1, "ATC_RAT_RLL_I") }
                    FactSlider { _fact: controller.getParameterFact(-1, "ATC_RAT_RLL_IMAX") }
                    FactSlider { _fact: controller.getParameterFact(-1, "ATC_RAT_RLL_D") }
                    FactSlider { _fact: controller.getParameterFact(-1, "ATC_RAT_YAW_P") }
                    FactSlider { _fact: controller.getParameterFact(-1, "ATC_RAT_YAW_I") }
                    FactSlider { _fact: controller.getParameterFact(-1, "ATC_RAT_YAW_IMAX") }
                    FactSlider { _fact: controller.getParameterFact(-1, "ATC_RAT_YAW_D") }

                } // Column - Position Controller Parameters
            } // Rectangle - Position Controller Parameters

            Rectangle {
                id: posParams
                visible: false
                anchors.left:       parent.left
                anchors.right:      parent.right
                height:             velColumn.height + _margins*2
                color:              palette.windowShade

                Column {
                    id: velColumn
                    anchors.margins:    _margins
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.top:        parent.top
                    spacing:            _margins*1.5

                    FactSlider { _fact: controller.getParameterFact(-1, "POS_XY_P") }
                    FactSlider { _fact: controller.getParameterFact(-1, "POS_Z_P") }
                    FactSlider { _fact: controller.getParameterFact(-1, "VEL_XY_P") }
                    FactSlider { _fact: controller.getParameterFact(-1, "VEL_XY_I") }
                    FactSlider { _fact: controller.getParameterFact(-1, "VEL_XY_IMAX") }
                    FactSlider { _fact: controller.getParameterFact(-1, "VEL_Z_P") }
                    FactSlider { _fact: controller.getParameterFact(-1, "ACCEL_Z_D") }
                    FactSlider { _fact: controller.getParameterFact(-1, "ACCEL_Z_FILT") }
                    FactSlider { _fact: controller.getParameterFact(-1, "ACCEL_Z_I") }
                    FactSlider { _fact: controller.getParameterFact(-1, "ACCEL_Z_IMAX") }
                    FactSlider { _fact: controller.getParameterFact(-1, "ACCEL_Z_P") }

                } // Column - VEL parameters
            } // Rectangle - VEL parameters

            Rectangle {
                id: navParams
                visible: false
                anchors.left:       parent.left
                anchors.right:      parent.right
                height:             wpnavColumn.height + _margins*2
                color:              palette.windowShade

                Column {
                    id:                 wpnavColumn
                    anchors.margins:    _margins
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.top:        parent.top
                    spacing:            _margins*1.5

                    FactSlider { _fact: controller.getParameterFact(-1, "WPNAV_ACCEL") }
                    FactSlider { _fact: controller.getParameterFact(-1, "WPNAV_ACCEL_Z") }
                    FactSlider { _fact: controller.getParameterFact(-1, "WPNAV_LOIT_JERK") }
                    FactSlider { _fact: controller.getParameterFact(-1, "WPNAV_LOIT_MAXA") }
                    FactSlider { _fact: controller.getParameterFact(-1, "WPNAV_LOIT_MINA") }
                    FactSlider { _fact: controller.getParameterFact(-1, "WPNAV_LOIT_SPEED") }
                    FactSlider { _fact: controller.getParameterFact(-1, "WPNAV_RADIUS") }
                    FactSlider { _fact: controller.getParameterFact(-1, "WPNAV_SPEED") }
                    FactSlider { _fact: controller.getParameterFact(-1, "WPNAV_SPEED_DN") }
                    FactSlider { _fact: controller.getParameterFact(-1, "WPNAV_SPEED_UP") }

                } // Column - WPNAV parameters
            } // Rectangle - WPNAV parameters
        } // Column
    } // Component
} // SetupView

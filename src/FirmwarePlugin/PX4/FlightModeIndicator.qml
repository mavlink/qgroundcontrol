/****************************************************************************
 *
 * (c) 2009-2022 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts  1.15

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0
import QGroundControl.FactSystem            1.0
import QGroundControl.FactControls          1.0

RowLayout {
    id: _root
    spacing: 0

    property bool showIndicator: true

    property real fontPointSize: ScreenTools.largeFontPointSize
    property var  activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    RowLayout {
        Layout.fillWidth: true

        QGCColoredImage {
            id:         flightModeIcon
            width:      ScreenTools.defaultFontPixelWidth * 3
            height:     ScreenTools.defaultFontPixelHeight
            fillMode:   Image.PreserveAspectFit
            mipmap:     true
            color:      qgcPal.text
            source:     "/qmlimages/FlightModesComponentIcon.png"
        }

        QGCLabel {
            text:               activeVehicle ? activeVehicle.flightMode : qsTr("N/A", "No data to display")
            font.pointSize:     fontPointSize
            Layout.alignment:   Qt.AlignCenter

            MouseArea {
                anchors.fill:   parent
                onClicked:      mainWindow.showIndicatorDrawer(drawerComponent)
            }
        }
    }

    Component {
        id: drawerComponent

        ToolIndicatorPage {
            id:         mainLayout
            showExpand: true

            property var  activeVehicle:            QGroundControl.multiVehicleManager.activeVehicle
            property Fact mpcLandSpeedFact:         controller.getParameterFact(-1, "MPC_LAND_SPEED", false)
            property Fact precisionLandingFact:     controller.getParameterFact(-1, "RTL_PLD_MD", false)
            property Fact sys_vehicle_resp:         controller.getParameterFact(-1, "SYS_VEHICLE_RESP", false)
            property Fact mpc_xy_vel_all:           controller.getParameterFact(-1, "MPC_XY_VEL_ALL", false)
            property Fact mpc_z_vel_all:            controller.getParameterFact(-1, "MPC_Z_VEL_ALL", false)
            property var  qgcPal:                   QGroundControl.globalPalette
            property real margins:                  ScreenTools.defaultFontPixelHeight
            property real valueColumnWidth:         Math.max(editFieldWidth, precisionLandingCombo.implicitWidth)

            FactPanelController { id: controller }

            // Mode list
            contentItem: FlightModeToolIndicatorContentItem { }

            // Settings
            expandedItem: ColumnLayout {
                Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 60
                spacing:                margins / 2

                IndicatorPageGroupLayout {
                    Layout.fillWidth: true

                    GridLayout {
                        Layout.fillWidth:   true
                        columns:            2

                        QGCLabel {
                            Layout.fillWidth:   true;
                            text:               qsTr("RTL Altitude")
                        }
                        FactTextField {
                            fact:                   controller.getParameterFact(-1, "RTL_RETURN_ALT")
                            Layout.preferredWidth:  valueColumnWidth
                        }

                        QGCLabel {
                            id:                 landDescentLabel
                            Layout.fillWidth:   true
                            text:               qsTr("Land Descent Rate")
                            visible:            mpcLandSpeedFact && controller.vehicle && !controller.vehicle.fixedWing
                        }
                        FactTextField {
                            fact:                   mpcLandSpeedFact
                            Layout.preferredWidth:  valueColumnWidth
                            visible:                landDescentLabel.visible
                        }

                        QGCLabel {
                            Layout.fillWidth:   true;
                            text:               qsTr("Precision Landing")
                            visible:            precisionLandingCombo.visible
                        }
                        FactComboBox {
                            id:                     precisionLandingCombo
                            Layout.minimumWidth:    editFieldWidth
                            fact:                   precisionLandingFact
                            indexModel:             false
                            sizeToContents:         true
                            visible:                precisionLandingFact
                        }
                    }
                }

                IndicatorPageGroupLayout {
                    Layout.fillWidth:   true
                    visible:            sys_vehicle_resp

                    ColumnLayout {
                        Layout.fillWidth:   true

                        QGCCheckBoxSlider {
                            id:                 responsivenessCheckBox
                            Layout.fillWidth:   true
                            text:               qsTr("Overall Responsiveness")
                            checked:            sys_vehicle_resp && sys_vehicle_resp.value >= 0

                            onClicked: {
                                if (checked) {
                                    sys_vehicle_resp.value = Math.abs(sys_vehicle_resp.value)
                                } else {
                                    sys_vehicle_resp.value = -Math.abs(sys_vehicle_resp.value)
                                }
                            }
                        }

                        FactSlider {
                            Layout.fillWidth:   true
                            enabled:            responsivenessCheckBox.checked
                            fact:               sys_vehicle_resp
                            from:               0.01
                            to:                 1
                            stepSize:           0.01
                        }

                        QGCLabel {
                            Layout.fillWidth:   true
                            enabled:            responsivenessCheckBox.checked
                            text:               qsTr("A higher value makes the vehicle react faster. Be aware that this affects braking as well, and a combination of slow responsiveness with high maximum velocity will lead to long braking distances.")
                            wrapMode:           QGCLabel.WordWrap
                        }
                        QGCLabel {
                            Layout.fillWidth:   true
                            visible:            sys_vehicle_resp && sys_vehicle_resp.value > 0.8
                            color:              qgcPal.warningText
                            text:               qsTr("Warning: a high responsiveness requires a vehicle with large thrust-to-weight ratio. The vehicle might lose altitude otherwise.")
                            wrapMode:           QGCLabel.WordWrap
                        }
                    }

                    Item {
                        Layout.fillWidth:   true
                        height:             1
                    }

                    ColumnLayout {
                        Layout.fillWidth:   true
                        visible:            mpc_xy_vel_all

                        QGCCheckBoxSlider {
                            id:                 xyVelCheckBox
                            Layout.fillWidth:   true
                            text:               qsTr("Overall Horizontal Velocity (m/s)")
                            checked:            mpc_xy_vel_all && mpc_xy_vel_all.value >= 0

                            onClicked: {
                                if (checked) {
                                    mpc_xy_vel_all.value = Math.abs(mpc_xy_vel_all.value)
                                } else {
                                    mpc_xy_vel_all.value = -Math.abs(mpc_xy_vel_all.value)
                                }
                            }
                        }

                        FactSlider {
                            Layout.fillWidth:   true
                            enabled:            xyVelCheckBox.checked
                            fact:               mpc_xy_vel_all
                            from:               0.5
                            to:                 20
                            stepSize:           0.5
                        }

                        Item {
                            Layout.fillWidth: true
                            height: 1
                        }

                        ColumnLayout {
                            Layout.fillWidth:   true
                            visible:            mpc_z_vel_all

                            QGCCheckBoxSlider {
                                id:                 zVelCheckBox
                                Layout.fillWidth:   true
                                text:               qsTr("Overall Vertical Velocity (m/s)")
                                checked:            mpc_z_vel_all && mpc_z_vel_all.value >= 0

                                onClicked: {
                                    if (checked) {
                                        mpc_z_vel_all.value = Math.abs(mpc_z_vel_all.value)
                                    } else {
                                        mpc_z_vel_all.value = -Math.abs(mpc_z_vel_all.value)
                                    }
                                }
                            }

                            FactSlider {
                                Layout.fillWidth:   true
                                enabled:            zVelCheckBox.checked
                                fact:               mpc_z_vel_all
                                from:               0.2
                                to:                 8
                                stepSize:           0.2
                            }
                        }
                    }
                }

                IndicatorPageGroupLayout {
                    Layout.fillWidth:  true

                    ColumnLayout {
                        Layout.fillWidth: true

                        QGCLabel { text: qsTr("Mission Turning Radius") }

                        FactSlider {
                            Layout.fillWidth:   true
                            fact:               controller.getParameterFact(-1, "NAV_ACC_RAD")
                            from:               2
                            to:                 16
                            stepSize:           0.5
                        }

                        QGCLabel {
                            Layout.fillWidth:   true
                            text:               qsTr("Increasing this leads to rounder turns in missions (corner cutting). Use the minimum value for accurate corner tracking.")
                            wrapMode:           QGCLabel.WordWrap
                        }
                    }
                }

                IndicatorPageGroupLayout {
                    Layout.fillWidth:   true
                    showDivider:        false

                    RowLayout {
                        Layout.fillWidth:  true

                        QGCLabel { Layout.fillWidth: true; text: qsTr("RC Transmitter Flight Modes") }
                        QGCButton {
                            text: qsTr("Configure")
                            onClicked: {
                                mainWindow.showVehicleSetupTool(qsTr("Radio"))
                                drawer.close()
                            }
                        }
                    }
                }
            }
        }
    }
}


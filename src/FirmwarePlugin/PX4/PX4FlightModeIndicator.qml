/****************************************************************************
 *
 * (c) 2009-2022 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.MultiVehicleManager
import QGroundControl.ScreenTools
import QGroundControl.Palette
import QGroundControl.FactSystem
import QGroundControl.FactControls

FlightModeIndicator {
    waitForParameters: true

    expandedPageComponent: Component {
        ColumnLayout {
            Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 60
            spacing:                margins / 2

            property Fact mpcLandSpeedFact:         controller.getParameterFact(-1, "MPC_LAND_SPEED", false)
            property Fact precisionLandingFact:     controller.getParameterFact(-1, "RTL_PLD_MD", false)
            property Fact sys_vehicle_resp:         controller.getParameterFact(-1, "SYS_VEHICLE_RESP", false)
            property Fact mpc_xy_vel_all:           controller.getParameterFact(-1, "MPC_XY_VEL_ALL", false)
            property Fact mpc_z_vel_all:            controller.getParameterFact(-1, "MPC_Z_VEL_ALL", false)
            property var  qgcPal:                   QGroundControl.globalPalette
            property real margins:                  ScreenTools.defaultFontPixelHeight
            property real valueColumnWidth:         Math.max(ScreenTools.implicitTextFieldWidth, precisionLandingCombo.implicitWidth)

            FactPanelController { id: controller }

            IndicatorPageGroupLayout {
                Layout.fillWidth: true

                IndicatorPageFactTextFieldRow {
                    Layout.fillWidth:           true;
                    label:                      qsTr("RTL Altitude")
                    fact:                       controller.getParameterFact(-1, "RTL_RETURN_ALT")
                    textFieldPreferredWidth:    valueColumnWidth
                }

                IndicatorPageFactTextFieldRow {
                    Layout.fillWidth:           true;
                    label:                      qsTr("Land Descent Rate")
                    fact:                       mpcLandSpeedFact
                    textFieldPreferredWidth:    valueColumnWidth
                    visible:                    mpcLandSpeedFact && controller.vehicle && !controller.vehicle.fixedWing
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing:          ScreenTools.defaultFontPixelWidth * 2
                    visible:          precisionLandingFact

                    QGCLabel {
                        Layout.fillWidth:   true;
                        text:               qsTr("Precision Landing")
                    }
                    FactComboBox {
                        id:                     precisionLandingCombo
                        Layout.minimumWidth:    ScreenTools.implicitTextFieldWidth
                        fact:                   precisionLandingFact
                        indexModel:             false
                        sizeToContents:         true
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
        }
    }
}

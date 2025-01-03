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
            property real sliderWidth:              ScreenTools.defaultFontPixelWidth * 40

            FactPanelController { id: controller }

            SettingsGroupLayout {
                Layout.fillWidth: true

                FactSlider {
                    Layout.fillWidth:       true
                    Layout.preferredWidth:  sliderWidth
                    label:                  qsTr("RTL Altitude")
                    fact:                   controller.getParameterFact(-1, "RTL_RETURN_ALT")
                    to:                     fact.maxIsDefaultForType ? QGroundControl.unitsConversion.metersToAppSettingsVerticalDistanceUnits(121.92) : fact.max
                    majorTickStepSize:      10
                }

                FactSlider {
                    Layout.fillWidth:       true
                    label:                  qsTr("Land Descent Rate")
                    fact:                   mpcLandSpeedFact
                    to:                     fact.maxIsDefaultForType ? QGroundControl.unitsConversion.metersToAppSettingsVerticalDistanceUnits(4) : fact.max
                    majorTickStepSize:      0.5
                    visible:                mpcLandSpeedFact && controller.vehicle && !controller.vehicle.fixedWing
                }
            }

            SettingsGroupLayout {
                Layout.fillWidth:   true
                visible:            sys_vehicle_resp

                ColumnLayout {
                    Layout.fillWidth: true

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
                        majorTickStepSize:  0.1
                    }

                    QGCLabel {
                        Layout.preferredWidth:  sliderWidth
                        enabled:            responsivenessCheckBox.checked
                        text:               qsTr("A higher value makes the vehicle react faster. Be aware that this affects braking as well, and a combination of slow responsiveness with high maximum velocity will lead to long braking distances.")
                        font.pointSize:     ScreenTools.smallFontPointSize
                        wrapMode:           QGCLabel.WordWrap
                    }
                    QGCLabel {
                        Layout.preferredWidth:  sliderWidth
                        color:              qgcPal.warningText
                        text:               qsTr("Warning: a high responsiveness requires a vehicle with large thrust-to-weight ratio. The vehicle might lose altitude otherwise.")
                        font.pointSize:     ScreenTools.smallFontPointSize
                        wrapMode:           QGCLabel.WordWrap
                        visible:            sys_vehicle_resp && sys_vehicle_resp.value > 0.8
                    }
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
                        majorTickStepSize:  0.5
                    }
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
                        majorTickStepSize:  0.5
                    }
                }
            }

            SettingsGroupLayout {
                Layout.fillWidth:   true
                showDividers:      false

                FactSlider {
                    Layout.fillWidth:   true
                    label:              qsTr("Mission Turning Radius")
                    fact:               controller.getParameterFact(-1, "NAV_ACC_RAD")
                    from:               2
                    to:                 16
                    majorTickStepSize:  2
                }

                QGCLabel {
                    Layout.fillWidth:       true
                    Layout.preferredWidth:  sliderWidth
                    text:                   qsTr("Increasing this leads to rounder turns in missions (corner cutting). Use the minimum value for accurate corner tracking.")
                    font.pointSize:         ScreenTools.smallFontPointSize
                    wrapMode:               QGCLabel.WordWrap
                }
            }
        }
    }
}

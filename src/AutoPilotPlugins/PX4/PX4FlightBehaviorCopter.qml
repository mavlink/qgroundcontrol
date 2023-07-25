/****************************************************************************
 *
 * (c) 2021 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

SetupPage {
    id:             flightBehavior
    pageComponent:  pageComponent
    property real _margins: ScreenTools.defaultFontPixelHeight

    FactPanelController {
        id:         controller
    }

    QGCPalette {
        id:                 qgcPal
    }

    property Fact _sys_vehicle_resp:  controller.getParameterFact(-1, "SYS_VEHICLE_RESP", false)
    property Fact _mpc_xy_vel_all:    controller.getParameterFact(-1, "MPC_XY_VEL_ALL", false)
    property Fact _mpc_z_vel_all:     controller.getParameterFact(-1, "MPC_Z_VEL_ALL", false)

    Component {
        id: pageComponent

        Column {

            spacing:            _margins

            Column {
                visible:                _sys_vehicle_resp

                QGCCheckBox {
                    id:                 responsivenessCheckbox
                    text:               qsTr("Enable responsiveness slider (if enabled, acceleration limit parameters and others are automatically set)")
                    checked:            _sys_vehicle_resp && _sys_vehicle_resp.value >= 0
                    onClicked: {
                        if (checked) {
                            _sys_vehicle_resp.value = Math.abs(_sys_vehicle_resp.value)
                        } else {
                            _sys_vehicle_resp.value = -Math.abs(_sys_vehicle_resp.value)
                        }
                    }
                }

                FactSliderPanel {
                    width:          availableWidth
                    enabled:        responsivenessCheckbox.checked

                    sliderModel: ListModel {
                        id:             responsivenessSlider

                        ListElement {
                            title:          qsTr("Responsiveness")
                            description:    qsTr("A higher value makes the vehicle react faster. Be aware that this affects braking as well, and a combination of slow responsiveness with high maximum velocity will lead to long braking distances.")
                            param:          "SYS_VEHICLE_RESP"
                            min:            0.01
                            max:            1
                            step:           0.01
                        }
                    }
                }
                QGCLabel {
                    visible:            _sys_vehicle_resp && _sys_vehicle_resp.value > 0.8
                    color:              qgcPal.warningText
                    text:              qsTr("Warning: a high responsiveness requires a vehicle with large thrust-to-weight ratio. The vehicle might lose altitude otherwise.")
                }
            }

            Column {
                visible:                _mpc_xy_vel_all

                QGCCheckBox {
                    id:                 xyVelCheckbox
                    text:               qsTr("Enable horizontal velocity slider (if enabled, individual velocity limit parameters are automatically set)")
                    checked:            _mpc_xy_vel_all ? (_mpc_xy_vel_all.value >= 0) : false
                    onClicked: {
                        if (checked) {
                            _mpc_xy_vel_all.value = Math.abs(_mpc_xy_vel_all.value)
                        } else {
                            _mpc_xy_vel_all.value = -Math.abs(_mpc_xy_vel_all.value)
                        }
                    }
                }

                FactSliderPanel {
                    width:          availableWidth
                    enabled:        xyVelCheckbox.checked

                    sliderModel: ListModel {
                        id:             xyVelSlider

                        ListElement {
                            title:          qsTr("Horizontal velocity (m/s)")
                            description:    qsTr("Limit the horizonal velocity (applies to all modes).")
                            param:          "MPC_XY_VEL_ALL"
                            min:            0.5
                            max:            20
                            step:           0.5
                        }
                    }
                }
            }

            Column {
                visible:                _mpc_z_vel_all

                QGCCheckBox {
                    id:                 zVelCheckbox
                    text:               qsTr("Enable vertical velocity slider (if enabled, individual velocity limit parameters are automatically set)")
                    checked:            _mpc_z_vel_all && _mpc_z_vel_all.value >= 0
                    onClicked: {
                        if (checked) {
                            _mpc_z_vel_all.value = Math.abs(_mpc_z_vel_all.value)
                        } else {
                            _mpc_z_vel_all.value = -Math.abs(_mpc_z_vel_all.value)
                        }
                    }
                }

                FactSliderPanel {
                    width:          availableWidth
                    enabled:        zVelCheckbox.checked

                    sliderModel: ListModel {
                        id:             zVelSlider

                        ListElement {
                            title:          qsTr("Vertical velocity (m/s)")
                            description:    qsTr("Limit the vertical velocity (applies to all modes).")
                            param:          "MPC_Z_VEL_ALL"
                            min:            0.2
                            max:            8
                            step:           0.2
                        }
                    }
                }
            }

            FactSliderPanel {
                width:          availableWidth

                sliderModel: ListModel {
                    ListElement {
                        title:          qsTr("Mission Turning Radius")
                        description:    qsTr("Increasing this leads to rounder turns in missions (corner cutting). Use the minimum value for accurate corner tracking.")
                        param:          "NAV_ACC_RAD"
                        min:            2
                        max:            16
                        step:           0.5
                    }
                }
            }

        } // Column
    } // Component - pageComponent
} // SetupPage

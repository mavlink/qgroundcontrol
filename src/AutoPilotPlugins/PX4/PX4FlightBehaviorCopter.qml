/****************************************************************************
 *
 * (c) 2021 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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



SetupPage {
    id:             flightBehavior
    pageComponent:  pageComponent

    property real _margins: ScreenTools.defaultFontPixelHeight

    FactPanelController {
        id: controller
    }

    QGCPalette {
        id: qgcPal
    }

    property Fact _sys_vehicle_resp:  controller.getParameterFact(-1, "SYS_VEHICLE_RESP", false)
    property Fact _mpc_xy_vel_all:    controller.getParameterFact(-1, "MPC_XY_VEL_ALL", false)
    property Fact _mpc_z_vel_all:     controller.getParameterFact(-1, "MPC_Z_VEL_ALL", false)

    Component {
        id: pageComponent

        ColumnLayout {
            width:   Math.max(availableWidth, ScreenTools.defaultFontPixelWidth * 40)
            spacing: _margins

            ColumnLayout {
                Layout.fillWidth:   true
                visible:            _sys_vehicle_resp

                SettingsGroupLayout {
                    Layout.fillWidth:   true
                    enabled:            responsivenessCheckbox.checked
                    heading:            qsTr("Responsiveness")
                    headingDescription: qsTr("A higher value makes the vehicle react faster. Be aware that this affects braking as well, and a combination of slow responsiveness with high maximum velocity will lead to long braking distances.")

                    // It's a bit tricky to handle the fact that the parameter goes negative to signal disabled.
                    // We can't allow the slider to go negative, hence all the hoops to jump through with value and loadComplete.
                    ValueSlider {
                        Layout.fillWidth:   true
                        from:               0.0
                        to:                 1
                        majorTickStepSize:  0.01
                        value:              { value = _sys_vehicle_resp ? Math.abs(_sys_vehicle_resp.value) : (from + to) / 2 }
                        decimalPlaces:      _sys_vehicle_resp ? _sys_vehicle_resp.decimalPlaces : 0

                        property bool loadComplete: false

                        Component.onCompleted: loadComplete = true

                        onValueChanged: {
                            if (loadComplete && enabled) {
                                _sys_vehicle_resp.value = value
                            }
                        }
                    }
                }

                QGCCheckBox {
                    id:                 responsivenessCheckbox
                    Layout.fillWidth:   true
                    text:               qsTr("Enable responsiveness slider (if enabled, acceleration limit parameters and others are automatically set)")
                    checked:            _sys_vehicle_resp && _sys_vehicle_resp.value >= 0
                    onClicked:          _sys_vehicle_resp.value = checked ? Math.abs(_sys_vehicle_resp.value) : -Math.abs(_sys_vehicle_resp.value)
                }

                QGCLabel {
                    Layout.fillWidth:   true
                    visible:            _sys_vehicle_resp && _sys_vehicle_resp.value > 0.8
                    color:              qgcPal.warningText
                    text:               qsTr("Warning: a high responsiveness requires a vehicle with large thrust-to-weight ratio. The vehicle might lose altitude otherwise.")
                }
            }

            ColumnLayout {
                Layout.fillWidth:   true
                visible:            _mpc_xy_vel_all

                SettingsGroupLayout {
                    Layout.fillWidth:   true
                    enabled:            xyVelCheckbox.checked
                    heading:            qsTr("Horizontal velocity (m/s)")
                    headingDescription: qsTr("Limit the horizonal velocity (applies to all modes).")

                    // It's a bit tricky to handle the fact that the parameter goes negative to signal disabled.
                    // We can't allow the slider to go negative, hence all the hoops to jump through with value and loadComplete.
                    ValueSlider {
                        Layout.fillWidth:   true
                        from:               0.5
                        to:                 20
                        majorTickStepSize:  0.6
                        value:              { value = _mpc_xy_vel_all ? Math.abs(_mpc_xy_vel_all.value) : (from + to) / 2 }
                        decimalPlaces:      _mpc_xy_vel_all ? _mpc_xy_vel_all.decimalPlaces : 0

                        property bool loadComplete: false

                        Component.onCompleted: loadComplete = true

                        onValueChanged: {
                            if (loadComplete && enabled) {
                                _mpc_xy_vel_all.value = value
                            }
                        }
                    }
                }

                QGCCheckBox {
                    id:                 xyVelCheckbox
                    Layout.fillWidth:   true
                    text:               qsTr("Enable horizontal velocity slider (if enabled, individual velocity limit parameters are automatically set)")
                    checked:            _mpc_xy_vel_all ? (_mpc_xy_vel_all.value >= 0) : false
                    onClicked:          mpc_xy_vel_all.value = checked ? Math.abs(_mpc_xy_vel_all.value) : -Math.abs(_mpc_xy_vel_all.value)
                }
            }

            ColumnLayout {
                Layout.fillWidth:   true
                visible:            _mpc_z_vel_all

                SettingsGroupLayout {
                    Layout.fillWidth:   true
                    enabled:            zVelCheckbox.checked
                    heading:            qsTr("Vertical velocity (m/s)")
                    headingDescription: qsTr("Limit the vertical velocity (applies to all modes).")

                    // It's a bit tricky to handle the fact that the parameter goes negative to signal disabled.
                    // We can't allow the slider to go negative, hence all the hoops to jump through with value and loadComplete.
                    ValueSlider {
                        Layout.fillWidth:   true
                        from:               0.2
                        to:                 8
                        majorTickStepSize:  0.2
                        value:              { value = _mpc_z_vel_all ? Math.abs(_mpc_z_vel_all.value) : (from + to) / 2 }
                        decimalPlaces:      _mpc_z_vel_all ? _mpc_z_vel_all.decimalPlaces : 0

                        property bool loadComplete: false

                        Component.onCompleted: loadComplete = true

                        onValueChanged: {
                            if (loadComplete && enabled) {
                                _mpc_z_vel_all.value = value
                            }
                        }
                    }
                }

                QGCCheckBox {
                    id:                 zVelCheckbox
                    Layout.fillWidth:   true
                    text:               qsTr("Enable vertical velocity slider (if enabled, individual velocity limit parameters are automatically set)")
                    checked:            _mpc_z_vel_all && _mpc_z_vel_all.value >= 0
                    onClicked:          mpc_z_vel_all.value = checked ? Math.abs(_mpc_z_vel_all.value) : -Math.abs(_mpc_z_vel_all.value)
                }
            }

            SettingsGroupLayout {
                Layout.fillWidth:   true
                heading:            qsTr("Mission Turning Radius")
                headingDescription: qsTr("Increasing this leads to rounder turns in missions (corner cutting). Use the minimum value for accurate corner tracking.")

                FactSlider {
                    Layout.fillWidth:   true
                    from:               QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnits(2)
                    to:                 QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnits(16)
                    majorTickStepSize:  0.5
                    fact:               controller.getParameterFact(-1, "NAV_ACC_RAD")
                }
            }
        }
    }
}

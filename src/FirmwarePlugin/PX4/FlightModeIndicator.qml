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
import QtQuick.Layouts  1.11

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
        id:             drawerComponent

        RowLayout {
            id:         mainLayout
            spacing:    margins

            property bool showExpand: true // Tells main window to show the expand widget

            property var  activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

            property Fact mpcLandSpeedFact:         controller.getParameterFact(-1, "MPC_LAND_SPEED", false)
            property Fact precisionLandingFact:     controller.getParameterFact(-1, "RTL_PLD_MD", false)
            property Fact sys_vehicle_resp:         controller.getParameterFact(-1, "SYS_VEHICLE_RESP", false)
            property Fact mpc_xy_vel_all:           controller.getParameterFact(-1, "MPC_XY_VEL_ALL", false)
            property Fact mpc_z_vel_all:            controller.getParameterFact(-1, "MPC_Z_VEL_ALL", false)
            property var  qgcPal:                   QGroundControl.globalPalette
            property real margins:                  ScreenTools.defaultFontPixelHeight
            property var  flightModeSettings:       QGroundControl.settingsManager.flightModeSettings
            property var  hiddenFlightModesFact:    null
            property var  hiddenFlightModesList:    [] 

            FactPanelController { id: controller }

            Component.onCompleted: {
                if (activeVehicle.px4Firmware) {
                    hiddenFlightModesFact = flightModeSettings.px4HiddenFlightModes
                } else if (activeVehicle.apmFirmware) {
                    hiddenFlightModesFact = flightModeSettings.apmHiddenFlightModes
                } else {
                    modeEditCheckBox.enabled = false
                }
                // Split string into list of flight modes
                if (hiddenFlightModesFact) {
                    hiddenFlightModesList = hiddenFlightModesFact.value.split(",")
                }
            }

            // Mode list
            ColumnLayout {
                id:                 modeColumn
                Layout.alignment:   Qt.AlignTop
                spacing:            ScreenTools.defaultFontPixelWidth / 2

                QGCCheckBox {
                    id:                 modeEditCheckBox
                    Layout.alignment:   Qt.AlignHCenter
                    text:               qsTr("Edit")
                    visible:            enabled && expanded

                    onClicked: {
                        for (var i=0; i<modeRepeater.count; i++) {
                            var button = modeRepeater.itemAt(i)
                            if (modeEditCheckBox.checked) {
                                button.checked = !hiddenFlightModesList.find(item => { return item === button.text } )
                            } else {
                                button.checked = false
                            }
                        }
                    }
                }

                Repeater {
                    id:     modeRepeater
                    model:  activeVehicle ? activeVehicle.flightModes : []

                    QGCButton {
                        id:                 modeButton
                        text:               modelData
                        Layout.fillWidth:   true
                        checkable:          modeEditCheckBox.checked
                        visible:            modeEditCheckBox.checked || !hiddenFlightModesList.find(item => { return item === modelData } )

                        onClicked: {
                            if (modeEditCheckBox.checked) {
                                hiddenFlightModesList = []
                                for (var i=0; i<modeRepeater.count; i++) {
                                    var button = modeRepeater.itemAt(i)
                                    if (!button.checked) {
                                        hiddenFlightModesList.push(button.text)
                                    }
                                }
                                hiddenFlightModesFact.value = hiddenFlightModesList.join(",")
                            } else {
                                activeVehicle.flightMode = modelData
                                indicatorDrawer.close()
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillHeight:  true
                width:              1
                color:              QGroundControl.globalPalette.text
                visible:            expanded
            }

            // Settings
            Column {
                width:      ScreenTools.defaultFontPixelWidth * 80
                spacing:    margins / 2
                visible:    expanded

                RowLayout {
                    width: parent.width

                    QGCLabel { Layout.fillWidth: true; text: qsTr("RTL Altitude") }
                    FactTextField {
                        fact:                   controller.getParameterFact(-1, "RTL_RETURN_ALT")
                        Layout.minimumWidth:    editFieldWidth
                    }
                }

                RowLayout {
                    width:      parent.width
                    visible:    mpcLandSpeedFact && controller.vehicle && !controller.vehicle.fixedWing

                    QGCLabel { Layout.fillWidth: true; text: qsTr("Land Descent Rate:") }
                    FactTextField {
                        fact:                   mpcLandSpeedFact
                        Layout.minimumWidth:    editFieldWidth
                    }
                }

                RowLayout {
                    width:      parent.width
                    visible:    precisionLandingFact

                    QGCLabel { Layout.fillWidth: true; text: qsTr("Precision Landing") }
                    FactComboBox {
                        fact:                   precisionLandingFact
                        indexModel:             false
                        sizeToContents:         true
                    }
                }

                Rectangle {
                    height: 1
                    width:  parent.width
                    color:  QGroundControl.globalPalette.text
                }

                Column {
                    width:      parent.width
                    visible:    sys_vehicle_resp

                    RowLayout {
                        width: parent.width

                        QGCLabel { Layout.fillWidth: true; text: qsTr("Overall Responsiveness") }
                        QGCCheckBox {
                            id:         responsivenessCheckBox
                            checked:    sys_vehicle_resp && sys_vehicle_resp.value >= 0
                            onClicked: {
                                if (checked) {
                                    sys_vehicle_resp.value = Math.abs(sys_vehicle_resp.value)
                                } else {
                                    sys_vehicle_resp.value = -Math.abs(sys_vehicle_resp.value)
                                }
                            }
                        }
                    }

                    FactSlider {
                        width:      parent.width
                        enabled:    responsivenessCheckBox.checked
                        fact:       sys_vehicle_resp
                        from:       0.01
                        to:         1
                        stepSize:   0.01
                    }

                    QGCLabel {
                        width:      parent.width
                        enabled:    responsivenessCheckBox.checked
                        text:       qsTr("A higher value makes the vehicle react faster. Be aware that this affects braking as well, and a combination of slow responsiveness with high maximum velocity will lead to long braking distances.")
                        wrapMode:   QGCLabel.WordWrap
                    }
                    QGCLabel {
                        width:      parent.width
                        visible:    sys_vehicle_resp && sys_vehicle_resp.value > 0.8
                        color:      qgcPal.warningText
                        text:       qsTr("Warning: a high responsiveness requires a vehicle with large thrust-to-weight ratio. The vehicle might lose altitude otherwise.")
                        wrapMode:   QGCLabel.WordWrap
                    }
                }

                Item {
                    height: 1
                    width:  parent.width
                }

                Column {
                    width:      parent.width
                    visible:    mpc_xy_vel_all

                    RowLayout {
                        width:  parent.width

                        QGCLabel { Layout.fillWidth: true; text: qsTr("Overall Horizontal Velocity (m/s)") }
                        QGCCheckBox {
                            id:         xyVelCheckBox
                            checked:    mpc_xy_vel_all && mpc_xy_vel_all.value >= 0
                            onClicked: {
                                if (checked) {
                                    mpc_xy_vel_all.value = Math.abs(mpc_xy_vel_all.value)
                                } else {
                                    mpc_xy_vel_all.value = -Math.abs(mpc_xy_vel_all.value)
                                }
                            }
                        }
                    }

                    FactSlider {
                        width:      parent.width
                        enabled:    xyVelCheckBox.checked
                        fact:       mpc_xy_vel_all
                        from:       0.5
                        to:         20
                        stepSize:   0.5
                    }
                }

                Item {
                    height: 1
                    width:  parent.width
                }

                Column {
                    width:      parent.width
                    visible:    mpc_z_vel_all

                    RowLayout {
                        width:  parent.width

                        QGCLabel { Layout.fillWidth: true; text: qsTr("Overall Vertical Velocity (m/s)") }
                        QGCCheckBox {
                            id:         zVelCheckBox
                            checked:    mpc_z_vel_all && mpc_z_vel_all.value >= 0
                            onClicked: {
                                if (checked) {
                                    mpc_z_vel_all.value = Math.abs(mpc_z_vel_all.value)
                                } else {
                                    mpc_z_vel_all.value = -Math.abs(mpc_z_vel_all.value)
                                }
                            }
                        }
                    }

                    FactSlider {
                        width:      parent.width
                        enabled:    zVelCheckBox.checked
                        fact:       mpc_z_vel_all
                        from:       0.2
                        to:         8
                        stepSize:   0.2
                    }
                }

                Rectangle {
                    height: 1
                    width:  parent.width
                    color:  QGroundControl.globalPalette.text
                }

                Column {
                    width:      parent.width
                    visible:    mpc_z_vel_all

                    QGCLabel { text: qsTr("Mission Turning Radius") }

                    FactSlider {
                        width:      parent.width
                        fact:       controller.getParameterFact(-1, "NAV_ACC_RAD")
                        from:       2
                        to:         16
                        stepSize:   0.5
                    }

                    QGCLabel {
                        width:      parent.width
                        text:       qsTr("Increasing this leads to rounder turns in missions (corner cutting). Use the minimum value for accurate corner tracking.")
                        wrapMode:   QGCLabel.WordWrap
                    }
                }

                Rectangle {
                    height: 1
                    width:  parent.width
                    color:  QGroundControl.globalPalette.text
                }

                RowLayout {
                    width: parent.width

                    QGCLabel { Layout.fillWidth: true; text: qsTr("RC Transmitter Flight Modes") }
                    QGCButton {
                        text: qsTr("Configure")
                        onClicked: {                            
                            mainWindow.showVehicleSetupTool(qsTr("Radio"))
                            indicatorDrawer.close()
                        }
                    }
                }
            }
        }
    }
}

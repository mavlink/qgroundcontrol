import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.VehicleSetup
import QGroundControl.FactControls

SetupPage {
    pageComponent: pageComponent

    Component {
        id: pageComponent

        ColumnLayout {
            id: root
            spacing: ScreenTools.defaultFontPixelHeight / 2

            property Fact activeJoystickNameFact: QGroundControl.settingsManager.joystickManagerSettings.activeJoystickName
            property string activeJoystickName: activeJoystickNameFact.value
            property var availableJoystickNames: joystickManager.availableJoystickNames
            property var activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
            property var activeJoystick: joystickManager.activeJoystick

            RowLayout {
                spacing: ScreenTools.defaultFontPixelWidth
                visible: joystickCombo.visible || calibrationRequiredLabel.visible

                QGCComboBox {
                    id: joystickCombo
                    sizeToContents: true
                    visible: activeJoystickName !== ""

                    onActivated: (index) => { activeJoystickNameFact.rawValue = textAt(index) }

                    function _buildModel() {
                        const availableNames = joystickManager.availableJoystickNames || [];
                        const modelNames = [...availableNames];
                        console.log("Building joystick combo model. Available names:", availableNames, "Active joystick name:", activeJoystickName);
                        console.log("Initial model names:", modelNames);
                        if (activeJoystickName && !modelNames.includes(activeJoystickName)) {
                            console.log("Adding active joystick name to combo model:", activeJoystickName);
                            modelNames.push(activeJoystickName);
                        }
                        joystickCombo.model = modelNames;
                    }

                    function _selectActiveJoystick() {
                        let index = joystickCombo.find(activeJoystickName)
                        if (index === -1) {
                            console.warn("Internal error: Active joystick name not in combo", activeJoystickName)
                        } else {
                            joystickCombo.currentIndex = index
                        }
                    }

                    function _recalc() {
                        _buildModel()
                        _selectActiveJoystick()
                    }

                    Component.onCompleted: _recalc()
                    Connections { target: root; function onActiveJoystickNameChanged() { joystickCombo._recalc() } }
                    Connections { target: joystickManager; function onAvailableJoystickNamesChanged() { joystickCombo._recalc() } }
                }

                QGCCheckBox {
                    text: qsTr("Enable")
                    checked: joystickManager.joystickEnabledForVehicle(activeVehicle)

                    onClicked: joystickManager.setJoystickEnabledForVehicle(activeVehicle, checked)
                }

                QGCLabel {
                    font.pointSize: ScreenTools.smallFontPointSize
                    text: qsTr("Not currently available")
                    visible: !activeJoystick
                }

                QGCLabel {
                    id: calibrationRequiredLabel
                    text: qsTr("Requires Calibration")
                    visible: activeJoystick && !activeJoystick.calibrated
                }
            }

            Loader {
                id: remoteControlCalibrationLoader
                Layout.fillWidth: true
                sourceComponent: activeJoystick && !activeVehicle.armed ? remoteControlCalibrationComponent : null
            }

            Component {
                id: remoteControlCalibrationComponent

                RemoteControlCalibration {
                    id: remoteControlCalibration

                    controller: JoystickConfigController {
                        joystick: joystickManager.activeJoystick
                        statusText: remoteControlCalibration.statusText
                        cancelButton: remoteControlCalibration.cancelButton
                        nextButton: remoteControlCalibration.nextButton
                        joystickMode: true
                    }

                    additionalSetupComponent: _activeJoystick ? _additionalSetupComponent : null
                    additionalMonitorComponent: _activeJoystick ? _additionalMonitorComponent : null

                    property var _controller: controller
                    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
                    property var _activeJoystick: _controller.joystick
                    property bool _requiresCalibration: _activeJoystick ? !_activeJoystick.calibrated : false

                    Component.onCompleted: controller.start()

                    Component {
                        id: _additionalSetupComponent

                        ColumnLayout {
                            spacing: ScreenTools.defaultFontPixelHeight / 2
                            enabled: !controller.calibrating

                            QGCTabBar {
                                id: tabBar
                                Layout.fillWidth: true

                                QGCTabButton {
                                    text: qsTr("Buttons")
                                    checked: true
                                }

                                QGCTabButton {
                                    text: qsTr("Settings")
                                    checked: false
                                }
                            }

                            JoystickComponentButtons {
                                Layout.fillWidth: true
                                joystick: _activeJoystick
                                controller: _controller
                                visible: tabBar.currentIndex === 0
                            }

                            JoystickComponentSettings {
                                Layout.fillWidth: true
                                joystick: _activeJoystick
                                visible: tabBar.currentIndex === 1
                            }
                        }
                    }

                    Component {
                        id: _additionalMonitorComponent

                        JoystickComponentButtonMonitor {
                        }
                    }
                }
            }
        }
    }
}

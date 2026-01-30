import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

// Joystick Indicator
Item {
    id:             control
    width:          joystickIcon.width * 1.1
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property bool showIndicator:    _activeJoystick
    property var  _activeJoystick:  joystickManager.activeJoystick
    property bool _joystickEnabled: globals.activeVehicle && joystickManager.activeJoystickEnabledForActiveVehicle

    QGCPalette { id: qgcPal }

    Component {
        id: joystickInfoPage

        ToolIndicatorPage {
            showExpand: true

            contentComponent: SettingsGroupLayout {
                heading: _activeJoystick ? _activeJoystick.name : qsTr("Joystick")

                GridLayout {
                    columns:        2
                    columnSpacing:  ScreenTools.defaultFontPixelWidth * 2

                    QGCLabel { text: qsTr("Enabled:") }
                    QGCLabel {
                        text: {
                            if (!globals.activeVehicle)
                                return qsTr("No Vehicle")
                            return _joystickEnabled ? qsTr("Yes") : qsTr("No")
                        }
                        color: {
                            if (!globals.activeVehicle)
                                return qgcPal.buttonText
                            return _joystickEnabled ? qgcPal.buttonText : "orange"
                        }
                    }

                    QGCLabel { text: qsTr("Type:") }
                    QGCLabel {
                        text: _activeJoystick ? (_activeJoystick.isGamepad ? _activeJoystick.gamepadType || qsTr("Gamepad") : qsTr("Joystick")) : ""
                    }

                    QGCLabel {
                        text:    qsTr("Connection:")
                        visible: _activeJoystick && _activeJoystick.connectionType && _activeJoystick.connectionType !== "Unknown" && _activeJoystick.connectionType !== "Invalid"
                    }
                    QGCLabel {
                        text:    _activeJoystick ? _activeJoystick.connectionType : ""
                        visible: _activeJoystick && _activeJoystick.connectionType && _activeJoystick.connectionType !== "Unknown" && _activeJoystick.connectionType !== "Invalid"
                    }

                    QGCLabel { text: qsTr("Inputs:") }
                    QGCLabel {
                        text: {
                            if (!_activeJoystick) return ""
                            var parts = [qsTr("%1 axes").arg(_activeJoystick.axisCount), qsTr("%1 buttons").arg(_activeJoystick.buttonCount)]
                            if (_activeJoystick.ballCount > 0) parts.push(qsTr("%1 balls").arg(_activeJoystick.ballCount))
                            if (_activeJoystick.touchpadCount() > 0) parts.push(qsTr("%1 touchpads").arg(_activeJoystick.touchpadCount()))
                            return parts.join(", ")
                        }
                    }

                    QGCLabel {
                        text:    qsTr("Battery:")
                        visible: _activeJoystick && _activeJoystick.batteryPercent >= 0
                    }
                    QGCLabel {
                        text: {
                            if (!_activeJoystick || _activeJoystick.batteryPercent < 0) return ""
                            var batteryText = qsTr("%1%").arg(_activeJoystick.batteryPercent)
                            if (_activeJoystick.powerState) batteryText += " (" + _activeJoystick.powerState + ")"
                            return batteryText
                        }
                        color:   _activeJoystick && _activeJoystick.batteryPercent < 20 ? "red" : qgcPal.buttonText
                        visible: _activeJoystick && _activeJoystick.batteryPercent >= 0
                    }

                    QGCLabel {
                        text:    qsTr("Features:")
                        visible: _activeJoystick && (_activeJoystick.hasRumble || _activeJoystick.hasLED || _activeJoystick.hasGyroscope() || _activeJoystick.hasAccelerometer())
                    }
                    QGCLabel {
                        property var features: {
                            var list = []
                            if (_activeJoystick) {
                                if (_activeJoystick.hasRumble) list.push(qsTr("Rumble"))
                                if (_activeJoystick.hasRumbleTriggers) list.push(qsTr("Trigger Rumble"))
                                if (_activeJoystick.hasLED) list.push(qsTr("LED"))
                                if (_activeJoystick.hasGyroscope()) list.push(qsTr("Gyro"))
                                if (_activeJoystick.hasAccelerometer()) list.push(qsTr("Accel"))
                            }
                            return list.join(", ")
                        }
                        text:    features
                        visible: _activeJoystick && (_activeJoystick.hasRumble || _activeJoystick.hasLED || _activeJoystick.hasGyroscope() || _activeJoystick.hasAccelerometer())
                    }

                    QGCLabel {
                        text:    qsTr("Player:")
                        visible: _activeJoystick && _activeJoystick.playerIndex >= 0
                    }
                    QGCLabel {
                        text:    _activeJoystick ? (_activeJoystick.playerIndex + 1).toString() : ""
                        visible: _activeJoystick && _activeJoystick.playerIndex >= 0
                    }
                }
            }

            expandedComponent: SettingsGroupLayout {
                heading: qsTr("Device Details")

                GridLayout {
                    columns:        2
                    columnSpacing:  ScreenTools.defaultFontPixelWidth * 2

                    QGCLabel {
                        text:    qsTr("Device Type:")
                        visible: _activeJoystick && _activeJoystick.deviceType
                    }
                    QGCLabel {
                        text:    _activeJoystick ? _activeJoystick.deviceType : ""
                        visible: _activeJoystick && _activeJoystick.deviceType
                    }

                    QGCLabel {
                        text:    qsTr("Vendor/Product:")
                        visible: _activeJoystick && _activeJoystick.vendorId > 0
                    }
                    QGCLabel {
                        text:    _activeJoystick ? "0x%1 / 0x%2".arg(_activeJoystick.vendorId.toString(16).toUpperCase().padStart(4, '0')).arg(_activeJoystick.productId.toString(16).toUpperCase().padStart(4, '0')) : ""
                        visible: _activeJoystick && _activeJoystick.vendorId > 0
                    }

                    QGCLabel {
                        text:    qsTr("Serial:")
                        visible: _activeJoystick && _activeJoystick.serial
                    }
                    QGCLabel {
                        text:    _activeJoystick ? _activeJoystick.serial : ""
                        visible: _activeJoystick && _activeJoystick.serial
                    }

                    QGCLabel {
                        text:    qsTr("Firmware:")
                        visible: _activeJoystick && _activeJoystick.firmwareVersion > 0
                    }
                    QGCLabel {
                        text:    _activeJoystick ? _activeJoystick.firmwareVersion.toString() : ""
                        visible: _activeJoystick && _activeJoystick.firmwareVersion > 0
                    }

                    QGCLabel {
                        text:    qsTr("Path:")
                        visible: _activeJoystick && _activeJoystick.path
                    }
                    QGCLabel {
                        text:    _activeJoystick ? _activeJoystick.path : ""
                        visible: _activeJoystick && _activeJoystick.path
                        elide:   Text.ElideMiddle
                        Layout.maximumWidth: ScreenTools.defaultFontPixelWidth * 20
                    }

                    QGCLabel {
                        text:    qsTr("GUID:")
                        visible: _activeJoystick && _activeJoystick.guid
                    }
                    QGCLabel {
                        text:    _activeJoystick ? _activeJoystick.guid : ""
                        visible: _activeJoystick && _activeJoystick.guid
                        font.family: "monospace"
                        font.pixelSize: ScreenTools.smallFontPointSize
                        elide:   Text.ElideMiddle
                        Layout.maximumWidth: ScreenTools.defaultFontPixelWidth * 20
                    }

                    QGCLabel {
                        text:    qsTr("Virtual:")
                        visible: _activeJoystick && _activeJoystick.isVirtual
                    }
                    QGCLabel {
                        text:    qsTr("Yes")
                        visible: _activeJoystick && _activeJoystick.isVirtual
                    }

                    QGCLabel {
                        text:    qsTr("LED Types:")
                        visible: _activeJoystick && (_activeJoystick.hasMonoLED() || _activeJoystick.hasRGBLED() || _activeJoystick.hasPlayerLED())
                    }
                    QGCLabel {
                        property var ledTypes: {
                            var list = []
                            if (_activeJoystick) {
                                if (_activeJoystick.hasMonoLED()) list.push(qsTr("Mono"))
                                if (_activeJoystick.hasRGBLED()) list.push(qsTr("RGB"))
                                if (_activeJoystick.hasPlayerLED()) list.push(qsTr("Player"))
                            }
                            return list.join(", ")
                        }
                        text:    ledTypes
                        visible: _activeJoystick && (_activeJoystick.hasMonoLED() || _activeJoystick.hasRGBLED() || _activeJoystick.hasPlayerLED())
                    }

                    QGCLabel {
                        text:    qsTr("Haptic:")
                        visible: _activeJoystick && _activeJoystick.hasHaptic()
                    }
                    QGCLabel {
                        text:    _activeJoystick && _activeJoystick.hasHaptic() ? qsTr("%1 effects").arg(_activeJoystick.hapticEffectsCount()) : ""
                        visible: _activeJoystick && _activeJoystick.hasHaptic()
                    }

                    QGCLabel {
                        text:    qsTr("Motion Sensors:")
                        visible: _activeJoystick && (_activeJoystick.hasGyroscope() || _activeJoystick.hasAccelerometer())
                    }
                    QGCLabel {
                        property var sensors: {
                            var list = []
                            if (_activeJoystick) {
                                if (_activeJoystick.hasGyroscope()) {
                                    var gyroRate = _activeJoystick.gyroscopeDataRate()
                                    list.push(gyroRate > 0 ? qsTr("Gyro (%1 Hz)").arg(gyroRate.toFixed(0)) : qsTr("Gyro"))
                                }
                                if (_activeJoystick.hasAccelerometer()) {
                                    var accelRate = _activeJoystick.accelerometerDataRate()
                                    list.push(accelRate > 0 ? qsTr("Accel (%1 Hz)").arg(accelRate.toFixed(0)) : qsTr("Accel"))
                                }
                            }
                            return list.join(", ")
                        }
                        text:    sensors
                        visible: _activeJoystick && (_activeJoystick.hasGyroscope() || _activeJoystick.hasAccelerometer())
                    }
                }
            }
        }
    }

    QGCColoredImage {
        id:                 joystickIcon
        width:              height
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        sourceSize.height:  height
        source:             "/qmlimages/Joystick.png"
        fillMode:           Image.PreserveAspectFit
        color: {
            if (!globals.activeVehicle) {
                return qgcPal.buttonText
            }
            if (_joystickEnabled) {
                return qgcPal.buttonText
            }
            return "orange"
        }
    }

    QGCMouseArea {
        fillItem:   joystickIcon
        onClicked:  mainWindow.showIndicatorDrawer(joystickInfoPage, control)
    }
}

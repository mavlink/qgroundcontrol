import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls

Item {
    anchors.fill: parent

    readonly property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    readonly property var _activeJoystick: joystickManager.activeJoystick

    Column {
        anchors.fill: parent

        VehicleSummaryRow {
            labelText: qsTr("Status")
            valueText: {
                if (!_activeJoystick) return qsTr("No joystick detected")
                if (_activeJoystick.axisCount === 0) return qsTr("Buttons only")
                if (!_activeJoystick.requiresCalibration) return qsTr("Ready")
                return _activeJoystick.settings.calibrated.rawValue ? qsTr("Calibrated") : qsTr("Needs calibration")
            }
        }

        VehicleSummaryRow {
            visible: _activeJoystick
            labelText: qsTr("Type")
            valueText: {
                if (!_activeJoystick) return ""
                if (_activeJoystick.isGamepad) {
                    return _activeJoystick.gamepadType || qsTr("Gamepad")
                }
                return qsTr("Joystick")
            }
        }

        VehicleSummaryRow {
            visible: _activeJoystick && _activeJoystick.connectionType
            labelText: qsTr("Connection")
            valueText: _activeJoystick ? _activeJoystick.connectionType : ""
        }

        VehicleSummaryRow {
            visible: _activeJoystick
            labelText: qsTr("Inputs")
            valueText: {
                if (!_activeJoystick) return ""
                var parts = []
                if (_activeJoystick.axisCount > 0) parts.push(qsTr("%1 axes").arg(_activeJoystick.axisCount))
                if (_activeJoystick.buttonCount > 0) parts.push(qsTr("%1 buttons").arg(_activeJoystick.buttonCount))
                if (_activeJoystick.ballCount > 0) parts.push(qsTr("%1 balls").arg(_activeJoystick.ballCount))
                if (_activeJoystick.touchpadCount() > 0) parts.push(qsTr("%1 touchpads").arg(_activeJoystick.touchpadCount()))
                return parts.join(", ")
            }
        }

        VehicleSummaryRow {
            visible: _activeJoystick && _activeJoystick.batteryPercent >= 0
            labelText: qsTr("Battery")
            valueText: {
                if (!_activeJoystick || _activeJoystick.batteryPercent < 0) return ""
                var text = qsTr("%1%").arg(_activeJoystick.batteryPercent)
                if (_activeJoystick.powerState) text += " (" + _activeJoystick.powerState + ")"
                return text
            }
            valueColor: _activeJoystick && _activeJoystick.batteryPercent < 20 ? "red" : ""
        }

        VehicleSummaryRow {
            visible: _activeJoystick && (_activeJoystick.hasRumble || _activeJoystick.hasLED || _activeJoystick.hasGyroscope() || _activeJoystick.hasAccelerometer())
            labelText: qsTr("Features")
            valueText: {
                if (!_activeJoystick) return ""
                var features = []
                if (_activeJoystick.hasRumble) features.push(qsTr("Rumble"))
                if (_activeJoystick.hasRumbleTriggers) features.push(qsTr("Triggers"))
                if (_activeJoystick.hasLED) features.push(qsTr("LED"))
                if (_activeJoystick.hasGyroscope()) features.push(qsTr("Gyro"))
                if (_activeJoystick.hasAccelerometer()) features.push(qsTr("Accel"))
                return features.join(", ")
            }
        }

        VehicleSummaryRow {
            visible: _activeJoystick && _activeJoystick.vendorId > 0
            labelText: qsTr("Device ID")
            valueText: _activeJoystick ? "0x%1:0x%2".arg(_activeJoystick.vendorId.toString(16).toUpperCase().padStart(4, '0')).arg(_activeJoystick.productId.toString(16).toUpperCase().padStart(4, '0')) : ""
        }

        VehicleSummaryRow {
            visible: _activeJoystick && _activeJoystick.playerIndex >= 0
            labelText: qsTr("Player")
            valueText: _activeJoystick ? (_activeJoystick.playerIndex + 1).toString() : ""
        }

        VehicleSummaryRow {
            visible: _activeJoystick && _activeJoystick.isVirtual
            labelText: qsTr("Virtual")
            valueText: qsTr("Yes")
        }
    }
}

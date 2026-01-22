import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

ColumnLayout {
    spacing: ScreenTools.defaultFontPixelHeight / 2

    readonly property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    readonly property var _activeJoystick: joystickManager.activeJoystick

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
        visible: _activeJoystick && _activeJoystick.axisCount > 0
        labelText: qsTr("Axes")
        valueText: _activeJoystick ? _activeJoystick.axisCount.toString() : "0"
    }

    VehicleSummaryRow {
        visible: _activeJoystick && _activeJoystick.buttonCount > 0
        labelText: qsTr("Buttons")
        valueText: _activeJoystick ? _activeJoystick.buttonCount.toString() : "0"
    }
}

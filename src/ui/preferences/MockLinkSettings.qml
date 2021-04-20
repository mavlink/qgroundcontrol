/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0

GridLayout {
    columns:        2
    rowSpacing:     _rowSpacing
    columnSpacing:  _colSpacing

    readonly property int _MAV_AUTOPILOT_GENERIC:       0
    readonly property int _MAV_AUTOPILOT_PX4:           12
    readonly property int _MAV_AUTOPILOT_ARDUPILOTMEGA: 3
    readonly property int _MAV_TYPE_FIXED_WING:         1
    readonly property int _MAV_TYPE_QUADROTOR:          2

    function saveSettings() {
        switch (firmwareTypeCombo.currentIndex) {
        case 0:
            subEditConfig.firmware = _MAV_AUTOPILOT_PX4
            break
        case 1:
            subEditConfig.firmware = _MAV_AUTOPILOT_ARDUPILOTMEGA
            if (vehicleTypeCombo.currentIndex === 1) {          // Hardcoded _MAV_TYPE_FIXED_WING
                subEditConfig.vehicle = _MAV_TYPE_FIXED_WING
            } else {
                subEditConfig.vehicle = _MAV_TYPE_QUADROTOR
            }
            break
        default:
            subEditConfig.firmware = _MAV_AUTOPILOT_GENERIC
            break
        }
        subEditConfig.sendStatus = sendStatus.checked
        subEditConfig.incrementVehicleId = incrementVehicleId.checked
    }

    Component.onCompleted: {
        switch (subEditConfig.firmware) {
        case _MAV_AUTOPILOT_PX4:
            firmwareTypeCombo.currentIndex = 0
            break
        case _MAV_AUTOPILOT_ARDUPILOTMEGA:
            firmwareTypeCombo.currentIndex = 1
            break
        default:
            firmwareTypeCombo.currentIndex = 2
            break
        }
        if (subEditConfig.vehicle === _MAV_TYPE_FIXED_WING) {          // Hardcoded _MAV_TYPE_FIXED_WING
            vehicleTypeCombo.currentIndex = 1
        } else {
            vehicleTypeCombo.currentIndex = 0
        }
    }

    QGCCheckBox {
        id:                 sendStatus
        Layout.columnSpan:  2
        text:               qsTr("Send Status Text and Voice")
        checked:            subEditConfig.sendStatus
    }

    QGCCheckBox {
        id:                 incrementVehicleId
        Layout.columnSpan:  2
        text:               qsTr("Increment Vehicle Id")
        checked:            subEditConfig.incrementVehicleId
    }

    QGCLabel { text: qsTr("Firmware") }
    QGCComboBox {
        id:                     firmwareTypeCombo
        Layout.preferredWidth:  _secondColumnWidth
        model:                  [ qsTr("PX4 Pro"), qsTr("ArduPilot"), qsTr("Generic MAVLink") ]

        property bool apmFirmwareSelected: currentIndex === 1
    }

    QGCLabel {
        text:       qsTr("Vehicle Type")
        visible:    firmwareTypeCombo.apmFirmwareSelected
    }
    QGCComboBox {
        id:                     vehicleTypeCombo
        Layout.preferredWidth:  _secondColumnWidth
        model:                  [ qsTr("ArduCopter"), qsTr("ArduPlane") ]
        visible:                firmwareTypeCombo.apmFirmwareSelected
    }
}

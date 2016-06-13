/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts  1.2

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

Item {
    id:     mockLinkSettings
    width:  parent ? parent.width : 0
    height: mockColumn.height

    function saveSettings() {
        if(px4Firmware.checked)
            subEditConfig.firmware = 12         // Hardcoded MAV_AUTOPILOT_PX4
        else if(apmFirmware.checked) {
            subEditConfig.firmware = 3
            if(planeVehicle.checked)
                subEditConfig.vehicle = 1       // Hardcoded MAV_TYPE_FIXED_WING
            else
                subEditConfig.vehicle = 2       // Hardcoded MAV_TYPE_QUADROTOR
        }
        else
            subEditConfig.firmware = 0
        subEditConfig.sendStatus = sendStatus.checked
    }

    Component.onCompleted: {
        if(subEditConfig.firmware === 12)       // Hardcoded MAV_AUTOPILOT_PX4
            px4Firmware.checked = true
        else if(subEditConfig.firmware === 3)   // Hardcoded MAV_AUTOPILOT_ARDUPILOTMEGA
            apmFirmware.checked = true
        else
            genericFirmware.checked = true
        if(subEditConfig.vehicle === 1)         // Hardcoded MAV_TYPE_FIXED_WING
            planeVehicle.checked = true
        else
            copterVehicle.checked = true
        sendStatus.checked = subEditConfig.sendStatus
    }

    Column {
        id:         mockColumn
        width:      mockLinkSettings.width
        spacing:    ScreenTools.defaultFontPixelHeight / 2
        QGCLabel {
            text:   qsTr("Mock Link Settings")
        }
        Item {
            height: ScreenTools.defaultFontPixelHeight / 2
            width:  parent.width
        }
        QGCCheckBox {
            id:         sendStatus
            text:       qsTr("Send Status Text and Voice")
            checked:    false
        }
        Item {
            height: ScreenTools.defaultFontPixelHeight / 2
            width:  parent.width
        }
        ColumnLayout {
            ExclusiveGroup { id: autoPilotGroup }
            QGCRadioButton {
                id:         px4Firmware
                text:       qsTr("PX4 Firmware")
                checked:    false
                exclusiveGroup: autoPilotGroup
            }
            QGCRadioButton {
                id:         apmFirmware
                text:       qsTr("APM Firmware")
                checked:    false
                exclusiveGroup: autoPilotGroup
            }
            QGCRadioButton {
                id:         genericFirmware
                text:       qsTr("Generic Firmware")
                checked:    false
                exclusiveGroup: autoPilotGroup
            }
        }
        Item {
            height: ScreenTools.defaultFontPixelHeight / 2
            width:  parent.width
        }
        QGCLabel {
            text:       qsTr("APM Vehicle Type")
            visible:    apmFirmware.checked
        }
        ColumnLayout {
            visible:    apmFirmware.checked
            ExclusiveGroup { id: apmVehicleGroup }
            QGCRadioButton {
                id:     copterVehicle
                text:   qsTr("ArduCopter")
                checked:    false
                exclusiveGroup: apmVehicleGroup
            }
            QGCRadioButton {
                id:     planeVehicle
                text:   qsTr("ArduPlane")
                checked:    false
                exclusiveGroup: apmVehicleGroup
            }
        }
    }
}

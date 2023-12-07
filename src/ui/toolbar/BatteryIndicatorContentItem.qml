/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.11
import QtQuick.Layouts  1.11

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0
import QGroundControl.FactSystem            1.0
import QGroundControl.FactControls          1.0
import MAVLink                              1.0

// This is the contentItem portion of the ToolIndicatorPage for the Battery toolbar item.
// It works for both PX4 and APM firmware.

ColumnLayout {
    id:         mainLayout
    spacing:    ScreenTools.defaultFontPixelHeight

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    Component {
        id: batteryValuesAvailableComponent

        QtObject {
            property bool functionAvailable:        battery.function.rawValue !== MAVLink.MAV_BATTERY_FUNCTION_UNKNOWN
            property bool showFunction:             functionAvailable && battery.function.rawValue != MAVLink.MAV_BATTERY_FUNCTION_ALL
            property bool temperatureAvailable:     !isNaN(battery.temperature.rawValue)
            property bool currentAvailable:         !isNaN(battery.current.rawValue)
            property bool mahConsumedAvailable:     !isNaN(battery.mahConsumed.rawValue)
            property bool timeRemainingAvailable:   !isNaN(battery.timeRemaining.rawValue)
            property bool chargeStateAvailable:     battery.chargeState.rawValue !== MAVLink.MAV_BATTERY_CHARGE_STATE_UNDEFINED
        }
    }

    QGCLabel {
        Layout.alignment:   Qt.AlignCenter
        text:               qsTr("Battery Status")
        font.family:        ScreenTools.demiboldFontFamily
    }

    RowLayout {
        spacing: ScreenTools.defaultFontPixelWidth

        ColumnLayout {
            Repeater {
                id:     col1Repeater
                model:  _activeVehicle ? _activeVehicle.batteries : 0

                ColumnLayout {
                    spacing: 0

                    property var batteryValuesAvailable: nameAvailableLoader.item

                    Loader {
                        id:                 nameAvailableLoader
                        sourceComponent:    batteryValuesAvailableComponent

                        property var battery: object
                    }

                    QGCLabel { text: qsTr("Battery %1").arg(object.id.rawValue);    visible: col1Repeater.count !== 1 }
                    QGCLabel { text: qsTr("Charge State");                          visible: batteryValuesAvailable.chargeStateAvailable }
                    QGCLabel { text: qsTr("Remaining");                             visible: batteryValuesAvailable.timeRemainingAvailable }
                    QGCLabel { text: qsTr("Remaining") }
                    QGCLabel { text: qsTr("Voltage") }
                    QGCLabel { text: qsTr("Consumed");                              visible: batteryValuesAvailable.mahConsumedAvailable }
                    QGCLabel { text: qsTr("Temperature");                           visible: batteryValuesAvailable.temperatureAvailable }
                    QGCLabel { text: qsTr("Function");                              visible: batteryValuesAvailable.showFunction }
                }
            }
        }

        ColumnLayout {
            Repeater {
                id:     col2Repeater
                model:  _activeVehicle ? _activeVehicle.batteries : 0

                ColumnLayout {
                    spacing: 0

                    property var batteryValuesAvailable: valueAvailableLoader.item

                    Loader {
                        id:                 valueAvailableLoader
                        sourceComponent:    batteryValuesAvailableComponent

                        property var battery: object
                    }

                    QGCLabel { text: "";                                                                        visible: col2Repeater.count !== 1 }
                    QGCLabel { text: object.chargeState.enumStringValue;                                        visible: batteryValuesAvailable.chargeStateAvailable }
                    QGCLabel { text: object.timeRemainingStr.value;                                             visible: batteryValuesAvailable.timeRemainingAvailable }
                    QGCLabel { text: object.percentRemaining.valueString + " " + object.percentRemaining.units }
                    QGCLabel { text: object.voltage.valueString + " " + object.voltage.units }
                    QGCLabel { text: object.mahConsumed.valueString + " " + object.mahConsumed.units;           visible: batteryValuesAvailable.mahConsumedAvailable }
                    QGCLabel { text: object.temperature.valueString + " " + object.temperature.units;           visible: batteryValuesAvailable.temperatureAvailable }
                    QGCLabel { text: object.function.enumStringValue;                                           visible: batteryValuesAvailable.showFunction }
                }
            }
        }
    }
}

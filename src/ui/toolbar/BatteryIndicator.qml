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
import MAVLink                              1.0

//-------------------------------------------------------------------------
//-- Battery Indicator
Item {
    id:             _root
    anchors.top:    parent.top
    anchors.bottom: parent.bottom
    width:          batteryIndicatorRow.width

    property bool showIndicator: true

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    Row {
        id:             batteryIndicatorRow
        anchors.top:    parent.top
        anchors.bottom: parent.bottom

        Repeater {
            model: _activeVehicle ? _activeVehicle.batteries : 0

            Loader {
                anchors.top:        parent.top
                anchors.bottom:     parent.bottom
                sourceComponent:    batteryVisual

                property var battery: object
            }
        }
    }
    MouseArea {
        anchors.fill:   parent
        onClicked: {
            mainWindow.showIndicatorPopup(_root, batteryPopup)
        }
    }

    Component {
        id: batteryVisual

        Row {
            anchors.top:    parent.top
            anchors.bottom: parent.bottom

            function getBatteryColor() {
                switch (battery.chargeState.rawValue) {
                case MAVLink.MAV_BATTERY_CHARGE_STATE_OK:
                    return qgcPal.text
                case MAVLink.MAV_BATTERY_CHARGE_STATE_LOW:
                    return qgcPal.colorOrange
                case MAVLink.MAV_BATTERY_CHARGE_STATE_CRITICAL:
                case MAVLink.MAV_BATTERY_CHARGE_STATE_EMERGENCY:
                case MAVLink.MAV_BATTERY_CHARGE_STATE_FAILED:
                case MAVLink.MAV_BATTERY_CHARGE_STATE_UNHEALTHY:
                    return qgcPal.colorRed
                default:
                    return qgcPal.text
                }
            }

            function getBatteryPercentageText() {
                if (!isNaN(battery.percentRemaining.rawValue)) {
                    if (battery.percentRemaining.rawValue > 98.9) {
                        return qsTr("100%")
                    } else {
                        return battery.percentRemaining.valueString + battery.percentRemaining.units
                    }
                } else if (!isNaN(battery.voltage.rawValue)) {
                    return battery.voltage.valueString + battery.voltage.units
                } else if (battery.chargeState.rawValue !== MAVLink.MAV_BATTERY_CHARGE_STATE_UNDEFINED) {
                    return battery.chargeState.enumStringValue
                }
                return ""
            }

            QGCColoredImage {
                anchors.top:        parent.top
                anchors.bottom:     parent.bottom
                width:              height
                sourceSize.width:   width
                source:             "/qmlimages/Battery.svg"
                fillMode:           Image.PreserveAspectFit
                color:              getBatteryColor()
            }

            QGCLabel {
                text:                   getBatteryPercentageText()
                font.pointSize:         ScreenTools.mediumFontPointSize
                color:                  getBatteryColor()
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    Component {
        id: batteryValuesAvailableComponent

        QtObject {
            property bool functionAvailable:        battery.function.rawValue !== MAVLink.MAV_BATTERY_FUNCTION_UNKNOWN
            property bool temperatureAvailable:     !isNaN(battery.temperature.rawValue)
            property bool currentAvailable:         !isNaN(battery.current.rawValue)
            property bool mahConsumedAvailable:     !isNaN(battery.mahConsumed.rawValue)
            property bool timeRemainingAvailable:   !isNaN(battery.timeRemaining.rawValue)
            property bool chargeStateAvailable:     battery.chargeState.rawValue !== MAVLink.MAV_BATTERY_CHARGE_STATE_UNDEFINED
        }
    }

    Component {
        id: batteryPopup

        Rectangle {
            width:          mainLayout.width   + mainLayout.anchors.margins * 2
            height:         mainLayout.height  + mainLayout.anchors.margins * 2
            radius:         ScreenTools.defaultFontPixelHeight / 2
            color:          qgcPal.window
            border.color:   qgcPal.text

            ColumnLayout {
                id:                 mainLayout
                anchors.margins:    ScreenTools.defaultFontPixelWidth
                anchors.top:        parent.top
                anchors.right:      parent.right
                spacing:            ScreenTools.defaultFontPixelHeight

                QGCLabel {
                    Layout.alignment:   Qt.AlignCenter
                    text:               qsTr("Battery Status")
                    font.family:        ScreenTools.demiboldFontFamily
                }

                RowLayout {
                    spacing: ScreenTools.defaultFontPixelWidth

                    ColumnLayout {
                        Repeater {
                            model: _activeVehicle ? _activeVehicle.batteries : 0

                            ColumnLayout {
                                spacing: 0

                                property var batteryValuesAvailable: nameAvailableLoader.item

                                Loader {
                                    id:                 nameAvailableLoader
                                    sourceComponent:    batteryValuesAvailableComponent

                                    property var battery: object
                                }

                                QGCLabel { text: qsTr("Battery %1").arg(object.id.rawValue) }
                                QGCLabel { text: qsTr("Charge State");                          visible: batteryValuesAvailable.chargeStateAvailable }
                                QGCLabel { text: qsTr("Remaining");                             visible: batteryValuesAvailable.timeRemainingAvailable }
                                QGCLabel { text: qsTr("Remaining") }
                                QGCLabel { text: qsTr("Voltage") }
                                QGCLabel { text: qsTr("Consumed");                              visible: batteryValuesAvailable.mahConsumedAvailable }
                                QGCLabel { text: qsTr("Temperature");                           visible: batteryValuesAvailable.temperatureAvailable }
                                QGCLabel { text: qsTr("Function");                              visible: batteryValuesAvailable.functionAvailable }
                            }
                        }
                    }

                    ColumnLayout {
                        Repeater {
                            model: _activeVehicle ? _activeVehicle.batteries : 0

                            ColumnLayout {
                                spacing: 0

                                property var batteryValuesAvailable: valueAvailableLoader.item

                                Loader {
                                    id:                 valueAvailableLoader
                                    sourceComponent:    batteryValuesAvailableComponent

                                    property var battery: object
                                }

                                QGCLabel { text: "" }
                                QGCLabel { text: object.chargeState.enumStringValue;                                        visible: batteryValuesAvailable.chargeStateAvailable }
                                QGCLabel { text: object.timeRemainingStr.value;                                             visible: batteryValuesAvailable.timeRemainingAvailable }
                                QGCLabel { text: object.percentRemaining.valueString + " " + object.percentRemaining.units }
                                QGCLabel { text: object.voltage.valueString + " " + object.voltage.units }
                                QGCLabel { text: object.mahConsumed.valueString + " " + object.mahConsumed.units;           visible: batteryValuesAvailable.mahConsumedAvailable }
                                QGCLabel { text: object.temperature.valueString + " " + object.temperature.units;           visible: batteryValuesAvailable.temperatureAvailable }
                                QGCLabel { text: object.function.enumStringValue;                                           visible: batteryValuesAvailable.functionAvailable }
                            }
                        }
                    }
                }
            }
        }
    }
}

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
            mainWindow.showIndicatorDrawer(batteryPopup)
        }
    }

    Component {
        id: batteryPopup

        ToolIndicatorPage {
            showExpand: true

            property real _margins: ScreenTools.defaultFontPixelHeight

            FactPanelController { id: controller }

            contentItem: BatteryIndicatorContentItem { }
            
            expandedItem: ColumnLayout {
                spacing: ScreenTools.defaultFontPixelHeight / 2

                IndicatorPageGroupLayout {
                    Layout.fillWidth:   true
                    heading:            qsTr("Low Battery Failsafe")

                    GridLayout {
                        columns: 2
                        columnSpacing: ScreenTools.defaultFontPixelHeight

                        QGCLabel { text: qsTr("Battery Warn Level") }
                        FactTextField {
                            Layout.fillWidth:       true
                            Layout.preferredWidth:  editFieldWidth
                            fact:                   controller.getParameterFact(-1, "BAT_LOW_THR")
                        }

                        QGCLabel { text: qsTr("Battery Failsafe Level") }
                        FactTextField {
                            Layout.fillWidth:       true
                            Layout.preferredWidth:  editFieldWidth
                            fact:                   controller.getParameterFact(-1, "BAT_CRIT_THR")
                        }

                        QGCLabel { text: qsTr("Failsafe Action") }
                        FactComboBox {
                            Layout.fillWidth:       true
                            Layout.preferredWidth:  editFieldWidth
                            fact:                   controller.getParameterFact(-1, "COM_LOW_BAT_ACT")
                            indexModel:             false
                            sizeToContents:         true
                        }

                        QGCLabel { text: qsTr("Battery Emergency Level") }
                        FactTextField {
                            Layout.fillWidth:       true
                            Layout.preferredWidth:  editFieldWidth
                            fact:                   controller.getParameterFact(-1, "BAT_EMERGEN_THR")
                        }
                    }
                }

                IndicatorPageGroupLayout {
                    Layout.fillWidth:   true
                    showDivider:        false
                    
                    RowLayout {
                        Layout.fillWidth: true

                        QGCLabel { Layout.fillWidth: true; text: qsTr("Vehicle Power") }
                        QGCButton {
                            text: qsTr("Configure")
                            onClicked: {                            
                                mainWindow.showVehicleSetupTool(qsTr("Power"))
                                indicatorDrawer.close()
                            }
                        }
                    }
                }
            }
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
}

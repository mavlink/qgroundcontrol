/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.MultiVehicleManager
import QGroundControl.ScreenTools
import QGroundControl.Palette
import QGroundControl.FactSystem
import QGroundControl.FactControls
import MAVLink

//-------------------------------------------------------------------------
//-- Battery Indicator
Item {
    id:             control
    anchors.top:    parent.top
    anchors.bottom: parent.bottom
    width:          batteryIndicatorRow.width

    property bool       showIndicator:      true
    property bool       waitForParameters:  false   // UI won't show until parameters are ready
    property Component  expandedPageComponent

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property Fact   _indicatorDisplay:  QGroundControl.settingsManager.batteryIndicatorSettings.display
    property bool   _showPercentage:    _indicatorDisplay.rawValue === 0
    property bool   _showVoltage:       _indicatorDisplay.rawValue === 1
    property bool   _showBoth:          _indicatorDisplay.rawValue === 2

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
            mainWindow.showIndicatorDrawer(batteryPopup, control)
        }
    }

    Component {
        id: batteryPopup

        ToolIndicatorPage {
            showExpand:         expandedComponent ? true : false
            waitForParameters:  control.waitForParameters
            contentComponent:   batteryContentComponent
            expandedComponent:  batteryExpandedComponent
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
                return qsTr("n/a")
            }

           function getBatteryVoltageText() {
                if (!isNaN(battery.voltage.rawValue)) {
                    return battery.voltage.valueString + battery.voltage.units
                } else if (battery.chargeState.rawValue !== MAVLink.MAV_BATTERY_CHARGE_STATE_UNDEFINED) {
                    return battery.chargeState.enumStringValue
                }
                return qsTr("n/a")
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

           ColumnLayout {
                id:                     batteryInfoColumn
                anchors.top:            parent.top
                anchors.bottom:         parent.bottom
                spacing:                0

                QGCLabel {
                    Layout.alignment:       Qt.AlignHCenter
                    verticalAlignment:      Text.AlignVCenter
                    color:                  getBatteryColor()
                    text:                   getBatteryPercentageText()
                    font.pointSize:         _showBoth ? ScreenTools.defaultFontPointSize : ScreenTools.mediumFontPointSize
                    visible:                _showBoth || _showPercentage
                }

                QGCLabel {
                    Layout.alignment:       Qt.AlignHCenter
                    font.pointSize:         _showBoth ? ScreenTools.defaultFontPointSize : ScreenTools.mediumFontPointSize
                    color:                  getBatteryColor()
                    text:                   getBatteryVoltageText()
                    visible:                _showBoth || _showVoltage
                }
            }
        }
    }

    Component {
        id: batteryContentComponent

        ColumnLayout {
            spacing: ScreenTools.defaultFontPixelHeight / 2

            property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

            Component {
                id: batteryValuesAvailableComponent

                QtObject {
                    property bool functionAvailable:         battery.function.rawValue !== MAVLink.MAV_BATTERY_FUNCTION_UNKNOWN
                    property bool showFunction:              functionAvailable && battery.function.rawValue != MAVLink.MAV_BATTERY_FUNCTION_ALL
                    property bool temperatureAvailable:      !isNaN(battery.temperature.rawValue)
                    property bool currentAvailable:          !isNaN(battery.current.rawValue)
                    property bool mahConsumedAvailable:      !isNaN(battery.mahConsumed.rawValue)
                    property bool timeRemainingAvailable:    !isNaN(battery.timeRemaining.rawValue)
                    property bool percentRemainingAvailable: !isNaN(battery.percentRemaining.rawValue)
                    property bool chargeStateAvailable:      battery.chargeState.rawValue !== MAVLink.MAV_BATTERY_CHARGE_STATE_UNDEFINED
                }
            }

            Repeater {
                model: _activeVehicle ? _activeVehicle.batteries : 0

                SettingsGroupLayout {
                    heading:        qsTr("Battery %1").arg(_activeVehicle.batteries.length === 1 ? qsTr("Status") : object.id.rawValue)
                    contentSpacing: 0
                    showDividers:   false

                    property var batteryValuesAvailable: batteryValuesAvailableLoader.item

                    Loader {
                        id:                 batteryValuesAvailableLoader
                        sourceComponent:    batteryValuesAvailableComponent

                        property var battery: object
                    }

                    LabelledLabel {
                        label:  qsTr("Charge State")
                        labelText:  object.chargeState.enumStringValue
                        visible:    batteryValuesAvailable.chargeStateAvailable
                    }

                    LabelledLabel {
                        label:      qsTr("Remaining")
                        labelText:  object.timeRemainingStr.value
                        visible:    batteryValuesAvailable.timeRemainingAvailable
                    }

                    LabelledLabel {
                        label:      qsTr("Remaining")
                        labelText:  object.percentRemaining.valueString + " " + object.percentRemaining.units
                        visible:    batteryValuesAvailable.percentRemainingAvailable
                    }

                    LabelledLabel {
                        label:      qsTr("Voltage")
                        labelText:  object.voltage.valueString + " " + object.voltage.units
                    }

                    LabelledLabel {
                        label:      qsTr("Consumed")
                        labelText:  object.mahConsumed.valueString + " " + object.mahConsumed.units
                        visible:    batteryValuesAvailable.mahConsumedAvailable
                    }

                    LabelledLabel {
                        label:      qsTr("Temperature")
                        labelText:  object.temperature.valueString + " " + object.temperature.units
                        visible:    batteryValuesAvailable.temperatureAvailable
                    }

                    LabelledLabel {
                        label:      qsTr("Function")
                        labelText:  object.function.enumStringValue
                        visible:    batteryValuesAvailable.showFunction
                    }
                }
            }
        }
    }

    Component {
        id: batteryExpandedComponent

        ColumnLayout {
            spacing: ScreenTools.defaultFontPixelHeight / 2

            FactPanelController { id: controller }

            Loader {
                sourceComponent: expandedPageComponent
            }

            SettingsGroupLayout {
                Layout.fillWidth: true

                RowLayout {
                    Layout.fillWidth: true

                    QGCLabel { Layout.fillWidth: true; text: qsTr("Battery Display") }
                    FactComboBox {
                        id:             editModeCheckBox
                        fact:           QGroundControl.settingsManager.batteryIndicatorSettings.display
                        sizeToContents: true
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    QGCLabel { Layout.fillWidth: true; text: qsTr("Vehicle Power") }
                    QGCButton {
                        text: qsTr("Configure")
                        onClicked: {
                            mainWindow.showVehicleSetupTool(qsTr("Power"))
                            mainWindow.closeIndicatorDrawer()
                        }
                    }
                }
            }
        }
    }
}

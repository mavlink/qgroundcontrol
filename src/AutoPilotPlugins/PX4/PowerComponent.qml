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
import QtQuick.Dialogs  1.2
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0
import QGroundControl.PX4           1.0

// Note: This setup supports back compat on battery parameter naming
//  Older firmware: Single battery setup using BAT_* naming
//  Newer firmware: Multiple battery setup using BAT#_* naming, with indices starting at 1
SetupPage {
    id:             powerPage
    pageComponent:  pageComponent

    Component {
        id: pageComponent

        Item {
            width:  Math.max(availableWidth, innerColumn.width)
            height: innerColumn.height

            readonly property string    _highlightPrefix:           "<font color=\"" + qgcPal.warningText + "\">"
            readonly property string    _highlightSuffix:           "</font>"
            readonly property string    _batNCellsIndexedParamName: "BAT#_N_CELLS"

            property int    _textEditWidth:                 ScreenTools.defaultFontPixelWidth * 8
            property Fact   _uavcanEnable:                  controller.getParameterFact(-1, "UAVCAN_ENABLE", false)
            property bool   _indexedBatteryParamsAvailable: controller.parameterExists(-1, _batNCellsIndexedParamName.replace("#", 1))
            property int    _indexedBatteryParamCount:      getIndexedBatteryParamCount()

            function getIndexedBatteryParamCount() {
                var batteryIndex = 1
                do {
                    if (!controller.parameterExists(-1, _batNCellsIndexedParamName.replace("#", batteryIndex))) {
                        return batteryIndex - 1
                    }
                    batteryIndex++
                } while (true)
            }

            PowerComponentController {
                id: controller
                onOldFirmware:          mainWindow.showMessageDialog(qsTr("ESC Calibration"),           qsTr("%1 cannot perform ESC Calibration with this version of firmware. You will need to upgrade to a newer firmware.").arg(QGroundControl.appName))
                onNewerFirmware:        mainWindow.showMessageDialog(qsTr("ESC Calibration"),           qsTr("%1 cannot perform ESC Calibration with this version of firmware. You will need to upgrade %1.").arg(QGroundControl.appName))
                onDisconnectBattery:    mainWindow.showMessageDialog(qsTr("ESC Calibration failed"),    qsTr("You must disconnect the battery prior to performing ESC Calibration. Disconnect your battery and try again."))
                onConnectBattery:       { var dialog = mainWindow.showPopupDialogFromComponent(escCalibrationDlgComponent); dialog.disableAcceptButton() }
            }

            ColumnLayout {
                id:                         innerColumn
                anchors.horizontalCenter:   parent.horizontalCenter
                spacing:                    ScreenTools.defaultFontPixelHeight

                function drawArrowhead(ctx, x, y, radians)
                {
                    ctx.save();
                    ctx.beginPath();
                    ctx.translate(x,y);
                    ctx.rotate(radians);
                    ctx.moveTo(0,0);
                    ctx.lineTo(5,10);
                    ctx.lineTo(-5,10);
                    ctx.closePath();
                    ctx.restore();
                    ctx.fill();
                }

                function drawLineWithArrow(ctx, x1, y1, x2, y2)
                {
                    ctx.beginPath();
                    ctx.moveTo(x1, y1);
                    ctx.lineTo(x2, y2);
                    ctx.stroke();
                    var rd = Math.atan((y2 - y1) / (x2 - x1));
                    rd += ((x2 > x1) ? 90 : -90) * Math.PI/180;
                    drawArrowhead(ctx, x2, y2, rd);
                }

                Repeater {
                    id:     batterySetupRepeater
                    model:  _indexedBatteryParamsAvailable ? _indexedBatteryParamCount : 1

                    Loader {
                        sourceComponent: batterySetupComponent

                        property int    batteryIndex:           index + 1
                        property bool   showBatteryIndex:       batterySetupRepeater.count > 1
                        property bool   useIndexedParamNames:   _indexedBatteryParamsAvailable
                    }
                }


                QGCGroupBox {
                    Layout.fillWidth:   true
                    title:              qsTr("ESC PWM Minimum and Maximum Calibration")

                    ColumnLayout {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        spacing:        ScreenTools.defaultFontPixelWidth

                        QGCLabel {
                            color:              qgcPal.warningText
                            wrapMode:           Text.WordWrap
                            text:               qsTr("WARNING: Propellers must be removed from vehicle prior to performing ESC calibration.")
                            Layout.fillWidth:   true
                        }

                        QGCLabel {
                            text: qsTr("You must use USB connection for this operation.")
                        }

                        QGCButton {
                            text:       qsTr("Calibrate")
                            width:      ScreenTools.defaultFontPixelWidth * 20
                            onClicked:  controller.calibrateEsc()
                        }
                    }
                }

                QGCCheckBox {
                    id:         showUAVCAN
                    text:       qsTr("Show UAVCAN Settings")
                    checked:    _uavcanEnable ? _uavcanEnable.rawValue !== 0 : false
                }

                QGCGroupBox {
                    Layout.fillWidth:       true
                    title:                  qsTr("UAVCAN Bus Configuration")
                    visible:                showUAVCAN.checked

                    Row {
                        id:         uavCanConfigRow
                        spacing:    ScreenTools.defaultFontPixelWidth

                        FactComboBox {
                            id:                 _uavcanEnabledCheckBox
                            width:              ScreenTools.defaultFontPixelWidth * 20
                            fact:               _uavcanEnable
                            indexModel:         false
                        }

                        QGCLabel {
                            anchors.verticalCenter: parent.verticalCenter
                            text:                   qsTr("Change required restart")
                        }
                    }
                }

                QGCGroupBox {
                    Layout.fillWidth:       true
                    title:                  qsTr("UAVCAN Motor Index and Direction Assignment")
                    visible:                showUAVCAN.checked

                    ColumnLayout {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        spacing:        ScreenTools.defaultFontPixelWidth

                        QGCLabel {
                            wrapMode:           Text.WordWrap
                            color:              qgcPal.warningText
                            text:               qsTr("WARNING: Propellers must be removed from vehicle prior to performing UAVCAN ESC configuration.")
                            Layout.fillWidth:   true
                        }

                        QGCLabel {
                            wrapMode:           Text.WordWrap
                            text:               qsTr("ESC parameters will only be accessible in the editor after assignment.")
                            Layout.fillWidth:   true
                        }

                        QGCLabel {
                            wrapMode:           Text.WordWrap
                            text:               qsTr("Start the process, then turn each motor into its turn direction, in the order of their motor indices.")
                            Layout.fillWidth:   true
                        }

                        QGCButton {
                            text:       qsTr("Start Assignment")
                            width:      ScreenTools.defaultFontPixelWidth * 20
                            onClicked:  controller.startBusConfigureActuators()
                        }

                        QGCButton {
                            text:       qsTr("Stop Assignment")
                            width:      ScreenTools.defaultFontPixelWidth * 20
                            onClicked:  controller.stopBusConfigureActuators()
                        }
                    }
                }

            } // Column

            Component {
                id: batterySetupComponent

                QGCGroupBox {
                    Layout.fillWidth:   true
                    title:              qsTr("Battery ") + (showBatteryIndex ? batteryIndex : "")

                    property var _controller:   controller
                    property int _batteryIndex: batteryIndex

                    BatteryParams {
                        id:             batParams
                        controller:     _controller
                        batteryIndex:   _batteryIndex
                    }

                    property bool battVoltageDividerAvailable:  batParams.battVoltageDividerAvailable
                    property bool battAmpsPerVoltAvailable:     batParams.battAmpsPerVoltAvailable

                    property Fact battSource:           batParams.battSource
                    property Fact battNumCells:         batParams.battNumCells
                    property Fact battHighVolt:         batParams.battHighVolt
                    property Fact battLowVolt:          batParams.battLowVolt
                    property Fact battVoltLoadDrop:     batParams.battVoltLoadDrop
                    property Fact battVoltageDivider:   batParams.battVoltageDivider
                    property Fact battAmpsPerVolt:      batParams.battAmpsPerVolt

                    function getBatteryImage() {
                        switch(battNumCells.value) {
                        case 1:  return "/qmlimages/PowerComponentBattery_01cell.svg";
                        case 2:  return "/qmlimages/PowerComponentBattery_02cell.svg"
                        case 3:  return "/qmlimages/PowerComponentBattery_03cell.svg"
                        case 4:  return "/qmlimages/PowerComponentBattery_04cell.svg"
                        case 5:  return "/qmlimages/PowerComponentBattery_05cell.svg"
                        case 6:  return "/qmlimages/PowerComponentBattery_06cell.svg"
                        default: return "/qmlimages/PowerComponentBattery_01cell.svg";
                        }
                    }

                    ColumnLayout {

                        RowLayout {
                            spacing: ScreenTools.defaultFontPixelWidth
                            visible: battSource.rawValue == -1

                            QGCLabel { text:  qsTr("Source") }
                            FactComboBox {
                                width:          _textEditWidth
                                fact:           battSource
                                indexModel:     false
                                sizeToContents: true
                            }
                        }

                        GridLayout {
                            id:             batteryGrid
                            columns:        5
                            columnSpacing:  ScreenTools.defaultFontPixelWidth
                            visible:        battSource.rawValue != -1

                            QGCLabel { text:  qsTr("Source") }
                            FactComboBox {
                                width:          _textEditWidth
                                fact:           battSource
                                indexModel:     false
                                sizeToContents: true
                            }

                            QGCColoredImage {
                                Layout.rowSpan:         4
                                width:                  height * 0.75
                                height:                 100
                                sourceSize.height:      height
                                fillMode:               Image.PreserveAspectFit
                                smooth:                 true
                                color:                  qgcPal.text
                                cache:                  false
                                source:                 getBatteryImage(batteryIndex)
                            }

                            Item { width: 1; height: 1; Layout.columnSpan: 2 }

                            QGCLabel { text:  qsTr("Number of Cells (in Series)") }
                            FactTextField {
                                width:      _textEditWidth
                                fact:       battNumCells
                                showUnits:  true
                            }
                            QGCLabel { text: qsTr("Battery Max:") }
                            QGCLabel { text: (battNumCells.value * battHighVolt.value).toFixed(1) + ' V' }

                            QGCLabel { text: qsTr("Empty Voltage (per cell)") }
                            FactTextField {
                                width:      _textEditWidth
                                fact:       battLowVolt
                                showUnits:  true
                            }
                            QGCLabel { text: qsTr("Battery Min:") }
                            QGCLabel { text: (battNumCells.value * battLowVolt.value).toFixed(1) + ' V' }


                            QGCLabel { text: qsTr("Full Voltage (per cell)") }
                            FactTextField {
                                width:      _textEditWidth
                                fact:       battHighVolt
                                showUnits:  true
                            }
                            Item { width: 1; height: 1; Layout.columnSpan: 2 }

                            QGCLabel {
                                text:       qsTr("Voltage divider")
                                visible:    battVoltageDividerAvailable
                            }
                            FactTextField {
                                fact:       battVoltageDivider
                                visible:    battVoltageDividerAvailable
                            }
                            QGCButton {
                                text:       qsTr("Calculate")
                                visible:    battVoltageDividerAvailable
                                onClicked:  mainWindow.showPopupDialogFromComponent(calcVoltageDividerDlgComponent, { batteryIndex: _batteryIndex })
                            }
                            Item { width: 1; height: 1; Layout.columnSpan: 2; visible: battVoltageDividerAvailable }

                            QGCLabel {
                                Layout.columnSpan:  batteryGrid.columns
                                Layout.fillWidth:   true
                                font.pointSize:     ScreenTools.smallFontPointSize
                                wrapMode:           Text.WordWrap
                                text:               qsTr("If the battery voltage reported by the vehicle is largely different than the voltage read externally using a voltmeter you can adjust the voltage multiplier value to correct this. ") +
                                                    qsTr("Click the Calculate button for help with calculating a new value.")
                                visible:            battVoltageDividerAvailable
                            }
                            QGCLabel {
                                text:       qsTr("Amps per volt")
                                visible:    battAmpsPerVoltAvailable
                            }
                            FactTextField {
                                fact:       battAmpsPerVolt
                                visible:    battAmpsPerVoltAvailable
                            }
                            QGCButton {
                                text:       qsTr("Calculate")
                                visible:    battAmpsPerVoltAvailable
                                onClicked:  mainWindow.showPopupDialogFromComponent(calcAmpsPerVoltDlgComponent, { batteryIndex: _batteryIndex })
                            }
                            Item { width: 1; height: 1; Layout.columnSpan: 2; visible: battAmpsPerVoltAvailable }

                            QGCLabel {
                                Layout.columnSpan:  batteryGrid.columns
                                Layout.fillWidth:   true
                                font.pointSize:     ScreenTools.smallFontPointSize
                                wrapMode:           Text.WordWrap
                                text:               qsTr("If the current draw reported by the vehicle is largely different than the current read externally using a current meter you can adjust the amps per volt value to correct this. ") +
                                                    qsTr("Click the Calculate button for help with calculating a new value.")
                                visible:            battAmpsPerVoltAvailable
                            }

                            QGCCheckBox {
                                id:                 showAdvanced
                                Layout.columnSpan:  batteryGrid.columns
                                text:               qsTr("Show Advanced Settings")
                            }

                            QGCLabel {
                                text:       qsTr("Voltage Drop on Full Load (per cell)")
                                visible:    showAdvanced.checked
                            }
                            FactTextField {
                                id:         battDropField
                                fact:       battVoltLoadDrop
                                showUnits:  true
                                visible:    showAdvanced.checked
                            }
                            Item { width: 1; height: 1; Layout.columnSpan: 3; visible: showAdvanced.checked }

                            QGCLabel {
                                Layout.columnSpan:  batteryGrid.columns
                                Layout.fillWidth:   true
                                wrapMode:           Text.WordWrap
                                font.pointSize:     ScreenTools.smallFontPointSize
                                text:               qsTr("Batteries show less voltage at high throttle. Enter the difference in Volts between idle throttle and full ") +
                                                    qsTr("throttle, divided by the number of battery cells. Leave at the default if unsure. ") +
                                                    _highlightPrefix + qsTr("If this value is set too high, the battery might be deep discharged and damaged.") + _highlightSuffix
                                visible:            showAdvanced.checked
                            }

                            QGCLabel {
                                text:       qsTr("Compensated Minimum Voltage:")
                                visible:    showAdvanced.checked
                            }
                            QGCLabel {
                                text:       ((battNumCells.value * battLowVolt.value) - (battNumCells.value * battVoltLoadDrop.value)).toFixed(1) + qsTr(" V")
                                visible:    showAdvanced.checked
                            }
                            Item { width: 1; height: 1; Layout.columnSpan: 3; visible: showAdvanced.checked }
                        } // Grid
                    }
                } // QGCGroupBox - Battery settings
            } // Component - batterySetupComponent

            Component {
                id: calcVoltageDividerDlgComponent

                QGCPopupDialog {
                    title:   qsTr("Calculate Voltage Divider")
                    buttons: StandardButton.Close

                    property var        _controller:        controller
                    property FactGroup  _batteryFactGroup:  controller.vehicle.getFactGroup("battery" + (dialogProperties.batteryIndex - 1))

                    BatteryParams {
                        id:             batParams
                        controller:     _controller
                        batteryIndex:   dialogProperties.batteryIndex
                    }

                    ColumnLayout {
                        spacing: ScreenTools.defaultFontPixelHeight

                        QGCLabel {
                            Layout.preferredWidth:  gridLayout.width
                            wrapMode:               Text.WordWrap
                            text:                   qsTr("Measure battery voltage using an external voltmeter and enter the value below. Click Calculate to set the new voltage multiplier.")
                        }

                        GridLayout {
                            id:         gridLayout
                            columns:    2

                            QGCLabel { text: qsTr("Measured voltage:") }
                            QGCTextField { id: measuredVoltage }

                            QGCLabel { text: qsTr("Vehicle voltage:") }
                            QGCLabel { text: _batteryFactGroup.voltage.valueString }

                            QGCLabel { text: qsTr("Voltage divider:") }
                            FactLabel { fact: batParams.battVoltageDivider }
                        }

                        QGCButton {
                            text: qsTr("Calculate")

                            onClicked:  {
                                var measuredVoltageValue = parseFloat(measuredVoltage.text)
                                if (measuredVoltageValue === 0 || isNaN(measuredVoltageValue)) {
                                    return
                                }
                                var newVoltageDivider = (measuredVoltageValue * batParams.battVoltageDivider.value) / _batteryFactGroup.voltage.value
                                if (newVoltageDivider > 0) {
                                    batParams.battVoltageDivider.value = newVoltageDivider
                                }
                            }
                        }
                    } // Column
                } // QGCViewDialog
            } // Component - calcVoltageDividerDlgComponent

            Component {
                id: calcAmpsPerVoltDlgComponent

                QGCPopupDialog {
                    title:   qsTr("Calculate Amps per Volt")
                    buttons: StandardButton.Close

                    property var        _controller:        controller
                    property FactGroup  _batteryFactGroup:  controller.vehicle.getFactGroup("battery" + (dialogProperties.batteryIndex - 1))

                    BatteryParams {
                        id:             batParams
                        controller:     _controller
                        batteryIndex:   dialogProperties.batteryIndex
                    }

                    ColumnLayout {
                        spacing: ScreenTools.defaultFontPixelHeight

                        QGCLabel {
                            Layout.preferredWidth:  gridLayout.width
                            wrapMode:               Text.WordWrap
                            text:                   qsTr("Measure current draw using an external current meter and enter the value below. Click Calculate to set the new amps per volt value.")
                        }

                        GridLayout {
                            id:         gridLayout
                            columns:    2

                            QGCLabel { text: qsTr("Measured current:") }
                            QGCTextField { id: measuredCurrent }

                            QGCLabel { text: qsTr("Vehicle current:") }
                            QGCLabel { text: _batteryFactGroup.current.valueString }

                            QGCLabel { text: qsTr("Amps per volt:") }
                            FactLabel { fact: batParams.battAmpsPerVolt }
                        }

                        QGCButton {
                            text: qsTr("Calculate")

                            onClicked:  {
                                var measuredCurrentValue = parseFloat(measuredCurrent.text)
                                if (measuredCurrentValue === 0 || isNaN(measuredCurrentValue)) {
                                    return
                                }
                                var newAmpsPerVolt = (measuredCurrentValue * batParams.battAmpsPerVolt.value) / _batteryFactGroup.current.value
                                if (newAmpsPerVolt != 0) {
                                    batParams.battAmpsPerVolt.value = newAmpsPerVolt
                                }
                            }
                        }
                    }
                }
            }

            Component {
                id: escCalibrationDlgComponent

                QGCPopupDialog {
                    id:         popupDialog
                    title:      qsTr("ESC Calibration")
                    buttons:    StandardButton.Ok

                    Connections {
                        target: controller

                        onBatteryConnected:     textLabel.text = qsTr("Performing calibration. This will take a few seconds..")
                        onCalibrationFailed:    { popupDialog.enableAcceptButton(); textLabel.text = _highlightPrefix + qsTr("ESC Calibration failed. ") + _highlightSuffix + errorMessage }
                        onCalibrationSuccess:   { popupDialog.enableAcceptButton(); textLabel.text = qsTr("Calibration complete. You can disconnect your battery now if you like.") }
                    }

                    ColumnLayout {
                        QGCLabel {
                            id:                     textLabel
                            wrapMode:               Text.WordWrap
                            text:                   _highlightPrefix + qsTr("WARNING: Props must be removed from vehicle prior to performing ESC calibration.") + _highlightSuffix + qsTr(" Connect the battery now and calibration will begin.")
                            Layout.fillWidth:       true
                            Layout.maximumWidth:    mainWindow.width / 2
                        }
                    }
                }
            }
        } // Item
    } // Component
} // SetupPage

/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @brief Battery, propeller and magnetometer settings
///     @author Gus Grubba <mavlink@grubba.com>

import QtQuick          2.2
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2
import QtQuick.Layouts  1.2

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0

SetupPage {
    id:             powerPage
    pageComponent:  pageComponent

    Component {
        id: pageComponent

        Column {
            id:         innerColumn
            width:      availableWidth
            spacing:    ScreenTools.defaultFontPixelHeight * 0.5

            property int textEditWidth:    ScreenTools.defaultFontPixelWidth * 8

            property Fact battNumCells:         controller.getParameterFact(-1, "BAT_N_CELLS")
            property Fact battHighVolt:         controller.getParameterFact(-1, "BAT_V_CHARGED")
            property Fact battLowVolt:          controller.getParameterFact(-1, "BAT_V_EMPTY")
            property Fact battVoltLoadDrop:     controller.getParameterFact(-1, "BAT_V_LOAD_DROP")
            property Fact battVoltageDivider:   controller.getParameterFact(-1, "BAT_V_DIV")
            property Fact battAmpsPerVolt:      controller.getParameterFact(-1, "BAT_A_PER_V")
            property Fact uavcanEnable:         controller.getParameterFact(-1, "UAVCAN_ENABLE", false)

            readonly property string highlightPrefix:   "<font color=\"" + qgcPal.warningText + "\">"
            readonly property string highlightSuffix:   "</font>"


            function getBatteryImage()
            {
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

            PowerComponentController {
                id:         controller
                factPanel:  powerPage.viewPanel

                onOldFirmware:          showMessage(qsTr("ESC Calibration"), qsTr("QGroundControl cannot perform ESC Calibration with this version of firmware. You will need to upgrade to a newer firmware."), StandardButton.Ok)
                onNewerFirmware:        showMessage(qsTr("ESC Calibration"), qsTr("QGroundControl cannot perform ESC Calibration with this version of firmware. You will need to upgrade QGroundControl."), StandardButton.Ok)
                onBatteryConnected:     showMessage(qsTr("ESC Calibration"), qsTr("Performing calibration. This will take a few seconds.."), 0)
                onCalibrationFailed:    showMessage(qsTr("ESC Calibration failed"), errorMessage, StandardButton.Ok)
                onCalibrationSuccess:   showMessage(qsTr("ESC Calibration"), qsTr("Calibration complete. You can disconnect your battery now if you like."), StandardButton.Ok)
                onConnectBattery:       showMessage(qsTr("ESC Calibration"), highlightPrefix + qsTr("WARNING: Props must be removed from vehicle prior to performing ESC calibration.") + highlightSuffix + qsTr(" Connect the battery now and calibration will begin."), 0)
                onDisconnectBattery:    showMessage(qsTr("ESC Calibration failed"), qsTr("You must disconnect the battery prior to performing ESC Calibration. Disconnect your battery and try again."), StandardButton.Ok)
            }

            Component {
                id: calcVoltageDividerDlgComponent

                QGCViewDialog {
                    id: calcVoltageDividerDlg

                    QGCFlickable {
                        anchors.fill:   parent
                        contentHeight:  column.height
                        contentWidth:   column.width

                        Column {
                            id:         column
                            width:      calcVoltageDividerDlg.width
                            spacing:    ScreenTools.defaultFontPixelHeight

                            QGCLabel {
                                width:      parent.width
                                wrapMode:   Text.WordWrap
                                text:       "Measure battery voltage using an external voltmeter and enter the value below. Click Calculate to set the new voltage multiplier."
                            }

                            Grid {
                                columns: 2
                                spacing: ScreenTools.defaultFontPixelHeight / 2
                                verticalItemAlignment: Grid.AlignVCenter

                                QGCLabel {
                                    text: "Measured voltage:"
                                }
                                QGCTextField { id: measuredVoltage }

                                QGCLabel { text: "Vehicle voltage:" }
                                QGCLabel { text: controller.vehicle.battery.voltage.valueString }

                                QGCLabel { text: "Voltage divider:" }
                                FactLabel { fact: battVoltageDivider }
                            }

                            QGCButton {
                                text: "Calculate"

                                onClicked:  {
                                    var measuredVoltageValue = parseFloat(measuredVoltage.text)
                                    if (measuredVoltageValue == 0) {
                                        return
                                    }
                                    var newVoltageDivider = (measuredVoltageValue * battVoltageDivider.value) / controller.vehicle.battery.voltage.value
                                    if (newVoltageDivider != 0) {
                                        battVoltageDivider.value = newVoltageDivider
                                    }
                                }
                            }
                        } // Column
                    } // QGCFlickable
                } // QGCViewDialog
            } // Component - calcVoltageDividerDlgComponent

            Component {
                id: calcAmpsPerVoltDlgComponent

                QGCViewDialog {
                    id: calcAmpsPerVoltDlg

                    QGCFlickable {
                        anchors.fill:   parent
                        contentHeight:  column.height
                        contentWidth:   column.width

                        Column {
                            id:         column
                            width:      calcAmpsPerVoltDlg.width
                            spacing:    ScreenTools.defaultFontPixelHeight

                            QGCLabel {
                                width:      parent.width
                                wrapMode:   Text.WordWrap
                                text:       "Measure current draw using an external current meter and enter the value below. Click Calculate to set the new amps per volt value."
                            }

                            Grid {
                                columns: 2
                                spacing: ScreenTools.defaultFontPixelHeight / 2
                                verticalItemAlignment: Grid.AlignVCenter

                                QGCLabel {
                                    text: "Measured current:"
                                }
                                QGCTextField { id: measuredCurrent }

                                QGCLabel { text: "Vehicle current:" }
                                QGCLabel { text: controller.vehicle.battery.current.valueString }

                                QGCLabel { text: "Amps per volt:" }
                                FactLabel { fact: battAmpsPerVolt }
                            }

                            QGCButton {
                                text: "Calculate"

                                onClicked:  {
                                    var measuredCurrentValue = parseFloat(measuredCurrent.text)
                                    if (measuredCurrentValue == 0) {
                                        return
                                    }
                                    var newAmpsPerVolt = (measuredCurrentValue * battAmpsPerVolt.value) / controller.vehicle.battery.current.value
                                    if (newAmpsPerVolt != 0) {
                                        battAmpsPerVolt.value = newAmpsPerVolt
                                    }
                                }
                            }
                        } // Column
                    } // QGCFlickable
                } // QGCViewDialog
            } // Component - calcAmpsPerVoltDlgComponent


            QGCLabel {
                text: qsTr("Battery")
                font.family: ScreenTools.demiboldFontFamily
            }

            Rectangle {
                width:  parent.width
                height: batteryGrid.height + ScreenTools.defaultFontPixelHeight
                color:  qgcPal.windowShade

                GridLayout {
                    id:                 batteryGrid
                    anchors.margins:    ScreenTools.defaultFontPixelHeight / 2
                    anchors.left:       parent.left
                    anchors.top:        parent.top
                    columns:            5
                    columnSpacing:      ScreenTools.defaultFontPixelWidth

                    QGCLabel {
                        text:               qsTr("Number of Cells (in Series)")
                    }

                    FactTextField {
                        id:         cellsField
                        width:      textEditWidth
                        fact:       battNumCells
                        showUnits:  true
                    }

                    QGCColoredImage {
                        id:                     batteryImage
                        Layout.rowSpan:         3
                        width:                  height * 0.75
                        height:                 100
                        sourceSize.height:      height
                        fillMode:               Image.PreserveAspectFit
                        smooth:                 true
                        color:                  qgcPal.text
                        cache:                  false
                        source:                 getBatteryImage();
                    }

                    Item { width: 1; height: 1; Layout.columnSpan: 2 }

                    QGCLabel {
                        id:                 battHighLabel
                        text:               qsTr("Full Voltage (per cell)")
                    }

                    FactTextField {
                        id:         battHighField
                        width:      textEditWidth
                        fact:       battHighVolt
                        showUnits:  true
                    }

                    QGCLabel {
                        text:   qsTr("Battery Max:")
                    }

                    QGCLabel {
                        text:   (battNumCells.value * battHighVolt.value).toFixed(1) + ' V'
                    }

                    QGCLabel {
                        id:                 battLowLabel
                        text:               qsTr("Empty Voltage (per cell)")
                    }

                    FactTextField {
                        id:         battLowField
                        width:      textEditWidth
                        fact:       battLowVolt
                        showUnits:  true
                    }

                    QGCLabel {
                        text:   qsTr("Battery Min:")
                    }

                    QGCLabel {
                        text:   (battNumCells.value * battLowVolt.value).toFixed(1) + ' V'
                    }

                    QGCLabel {
                        text:               qsTr("Voltage divider")
                    }

                    FactTextField {
                        id:                 voltMultField
                        fact:               battVoltageDivider
                    }

                    QGCButton {
                        id:                 voltMultCalculateButton
                        text:               "Calculate"
                        onClicked:          showDialog(calcVoltageDividerDlgComponent, qsTr("Calculate Voltage Divider"), powerPage.showDialogDefaultWidth, StandardButton.Close)
                    }

                    Item { width: 1; height: 1; Layout.columnSpan: 2 }

                    QGCLabel {
                        id:                 voltMultHelp
                        Layout.columnSpan:  batteryGrid.columns
                        Layout.fillWidth:   true
                        font.pointSize:     ScreenTools.smallFontPointSize
                        wrapMode:           Text.WordWrap
                        text:               "If the battery voltage reported by the vehicle is largely different than the voltage read externally using a voltmeter you can adjust the voltage multiplier value to correct this. " +
                                            "Click the Calculate button for help with calculating a new value."
                    }

                    QGCLabel {
                        id:                 ampPerVoltLabel
                        text:               qsTr("Amps per volt")
                    }

                    FactTextField {
                        id:                 ampPerVoltField
                        fact:               battAmpsPerVolt
                    }

                    QGCButton {
                        id:                 ampPerVoltCalculateButton
                        text:               "Calculate"
                        onClicked:          showDialog(calcAmpsPerVoltDlgComponent, qsTr("Calculate Amps per Volt"), powerPage.showDialogDefaultWidth, StandardButton.Close)
                    }

                    Item { width: 1; height: 1; Layout.columnSpan: 2 }

                    QGCLabel {
                        id:                 ampPerVoltHelp
                        Layout.columnSpan:  batteryGrid.columns
                        Layout.fillWidth:   true
                        font.pointSize:     ScreenTools.smallFontPointSize
                        wrapMode:           Text.WordWrap
                        text:               "If the current draw reported by the vehicle is largely different than the current read externally using a current meter you can adjust the amps per volt value to correct this. " +
                                            "Click the Calculate button for help with calculating a new value."
                    }
                } // Grid
            } // Rectangle - Battery settings

            QGCLabel {
                text:           qsTr("ESC PWM Minimum and Maximum Calibration")
                font.family:    ScreenTools.demiboldFontFamily
            }

            Rectangle {
                width:  parent.width
                height: escCalColumn.height + ScreenTools.defaultFontPixelHeight
                color:  qgcPal.windowShade

                Column {
                    id :                escCalColumn
                    anchors.margins:    ScreenTools.defaultFontPixelHeight / 2
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.top:        parent.top
                    spacing:            ScreenTools.defaultFontPixelWidth

                    QGCLabel {
                        width:      parent.width
                        color:      qgcPal.warningText
                        wrapMode:   Text.WordWrap
                        text:       qsTr("WARNING: Propellers must be removed from vehicle prior to performing ESC calibration.")
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
                checked:    uavcanEnable.rawValue != 0
            }

            QGCLabel {
                text:           qsTr("UAVCAN Bus Configuration")
                font.family:    ScreenTools.demiboldFontFamily
                visible:        showUAVCAN.checked
            }

            Rectangle {
                width:      parent.width
                height:     uavCanConfigRow.height + ScreenTools.defaultFontPixelHeight
                color:      qgcPal.windowShade
                visible:    showUAVCAN.checked

                Row {
                    id:                 uavCanConfigRow
                    anchors.margins:    ScreenTools.defaultFontPixelHeight / 2
                    anchors.left:       parent.left
                    anchors.top:        parent.top
                    spacing:            ScreenTools.defaultFontPixelWidth

                    FactComboBox {
                        id:                 uavcanEnabledCheckBox
                        width:              ScreenTools.defaultFontPixelWidth * 20
                        fact:               uavcanEnable
                        indexModel:         false
                    }

                    QGCLabel {
                        anchors.verticalCenter: parent.verticalCenter
                        text:                   qsTr("Change required restart")
                    }
                }
            }

            QGCLabel {
                text:           qsTr("UAVCAN Motor Index and Direction Assignment")
                font.family:    ScreenTools.demiboldFontFamily
                visible:        showUAVCAN.checked
            }

            Rectangle {
                width:      parent.width
                height:     uavCanEscCalColumn.height + ScreenTools.defaultFontPixelHeight
                color:      qgcPal.windowShade
                visible:    showUAVCAN.checked
                enabled:    uavcanEnabledCheckBox.checked

                Column {
                    id:                 uavCanEscCalColumn
                    anchors.margins:    ScreenTools.defaultFontPixelHeight / 2
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.top:        parent.top
                    spacing:            ScreenTools.defaultFontPixelWidth

                    QGCLabel {
                        width:      parent.width
                        wrapMode:   Text.WordWrap
                        color:      qgcPal.warningText
                        text:       qsTr("WARNING: Propellers must be removed from vehicle prior to performing UAVCAN ESC configuration.")
                    }

                    QGCLabel {
                        width:      parent.width
                        wrapMode:   Text.WordWrap
                        text:       qsTr("ESC parameters will only be accessible in the editor after assignment.")
                    }

                    QGCLabel {
                        width:      parent.width
                        wrapMode:   Text.WordWrap
                        text:       qsTr("Start the process, then turn each motor into its turn direction, in the order of their motor indices.")
                    }

                    QGCButton {
                        text:       qsTr("Start Assignment")
                        width:      ScreenTools.defaultFontPixelWidth * 20
                        onClicked:  controller.busConfigureActuators()
                    }

                    QGCButton {
                        text:       qsTr("Stop Assignment")
                        width:      ScreenTools.defaultFontPixelWidth * 20
                        onClicked:  controller.stopBusConfigureActuators()
                    }
                }
            }

            QGCCheckBox {
                id:     showAdvanced
                text:   qsTr("Show Advanced Settings")
            }

            QGCLabel {
                text:           qsTr("Advanced Power Settings")
                font.family:    ScreenTools.demiboldFontFamily
                visible:        showAdvanced.checked
            }

            Rectangle {
                id:         batteryRectangle
                width:      parent.width
                height:     advBatteryColumn.height + ScreenTools.defaultFontPixelHeight
                color:      qgcPal.windowShade
                visible:    showAdvanced.checked

                Column {
                    id: advBatteryColumn
                    anchors.margins:    ScreenTools.defaultFontPixelHeight / 2
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.top:        parent.top
                    spacing:            ScreenTools.defaultFontPixelWidth

                    Row {
                        spacing: ScreenTools.defaultFontPixelWidth

                        QGCLabel {
                            text:               qsTr("Voltage Drop on Full Load (per cell)")
                            anchors.baseline:   battDropField.baseline
                        }

                        FactTextField {
                            id:         battDropField
                            width:      textEditWidth
                            fact:       battVoltLoadDrop
                            showUnits:  true
                        }
                    }

                    QGCLabel {
                        width:      parent.width
                        wrapMode:   Text.WordWrap
                        text:       qsTr("Batteries show less voltage at high throttle. Enter the difference in Volts between idle throttle and full ") +
                                    qsTr("throttle, divided by the number of battery cells. Leave at the default if unsure. ") +
                                    highlightPrefix + qsTr("If this value is set too high, the battery might be deep discharged and damaged.") + highlightSuffix
                    }

                    Row {
                        spacing: ScreenTools.defaultFontPixelWidth

                        QGCLabel {
                            text: qsTr("Compensated Minimum Voltage:")
                        }

                        QGCLabel {
                            text: ((battNumCells.value * battLowVolt.value) - (battNumCells.value * battVoltLoadDrop.value)).toFixed(1) + qsTr(" V")
                        }
                    }
                }
            } // Rectangle - Advanced power settings
        } // Column
    } // Component
} // SetupPage

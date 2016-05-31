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

import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Dialogs 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0
import QGroundControl.ScreenTools 1.0
import QGroundControl.Controllers 1.0

QGCView {
    id:         rootQGCView
    viewPanel:  panel

    property int textEditWidth:    ScreenTools.defaultFontPixelWidth * 8

    property Fact battNumCells:     controller.getParameterFact(-1, "BAT_N_CELLS")
    property Fact battHighVolt:     controller.getParameterFact(-1, "BAT_V_CHARGED")
    property Fact battLowVolt:      controller.getParameterFact(-1, "BAT_V_EMPTY")
    property Fact battVoltLoadDrop: controller.getParameterFact(-1, "BAT_V_LOAD_DROP")
    property Fact uavcanEnable:     controller.getParameterFact(-1, "UAVCAN_ENABLE", false)

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
        factPanel:  panel

        onOldFirmware:          showMessage(qsTr("ESC Calibration"), qsTr("QGroundControl cannot perform ESC Calibration with this version of firmware. You will need to upgrade to a newer firmware."), StandardButton.Ok)
        onNewerFirmware:        showMessage(qsTr("ESC Calibration"), qsTr("QGroundControl cannot perform ESC Calibration with this version of firmware. You will need to upgrade QGroundControl."), StandardButton.Ok)
        onBatteryConnected:     showMessage(qsTr("ESC Calibration"), qsTr("Performing calibration. This will take a few seconds.."), 0)
        onCalibrationFailed:    showMessage(qsTr("ESC Calibration failed"), errorMessage, StandardButton.Ok)
        onCalibrationSuccess:   showMessage(qsTr("ESC Calibration"), qsTr("Calibration complete. You can disconnect your battery now if you like."), StandardButton.Ok)
        onConnectBattery:       showMessage(qsTr("ESC Calibration"), highlightPrefix + qsTr("WARNING: Props must be removed from vehicle prior to performing ESC calibration.") + highlightSuffix + qsTr(" Connect the battery now and calibration will begin."), 0)
        onDisconnectBattery:    showMessage(qsTr("ESC Calibration failed"), qsTr("You must disconnect the battery prior to performing ESC Calibration. Disconnect your battery and try again."), StandardButton.Ok)
    }

    QGCPalette { id: palette; colorGroupEnabled: panel.enabled }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        QGCFlickable {
            anchors.fill:       parent
            clip:               true
            contentHeight:      innerColumn.height
            contentWidth:       panel.width
            flickableDirection: Flickable.VerticalFlick

            Column {
                id:             innerColumn
                width:          panel.width
                spacing:        ScreenTools.defaultFontPixelHeight * 0.5

                QGCLabel {
                    text: qsTr("Battery")
                    font.family: ScreenTools.demiboldFontFamily
                }

                Rectangle {
                    width:  parent.width
                    height: voltageCol.height + ScreenTools.defaultFontPixelHeight
                    color:  palette.windowShade

                    Column {
                        id:                 voltageCol
                        anchors.margins:    ScreenTools.defaultFontPixelHeight / 2
                        anchors.left:       parent.left
                        anchors.top:        parent.top
                        spacing:            ScreenTools.defaultFontPixelHeight / 2

                        property real firstColumnWidth: Math.max(Math.max(cellsLabel.contentWidth, battHighLabel.contentWidth), battLowLabel.contentWidth) + ScreenTools.defaultFontPixelWidth

                        Row {
                            spacing: ScreenTools.defaultFontPixelWidth

                            QGCLabel {
                                id:                 cellsLabel
                                text:               qsTr("Number of Cells (in Series)")
                                anchors.baseline:   cellsField.baseline
                            }

                            FactTextField {
                                id:         cellsField
                                x:          voltageCol.firstColumnWidth
                                width:      textEditWidth
                                fact:       battNumCells
                                showUnits: true
                            }
                        }

                        Row {
                            spacing: ScreenTools.defaultFontPixelWidth

                            QGCLabel {
                                id:                 battHighLabel
                                text:               qsTr("Full Voltage (per cell)")
                                anchors.baseline:   battHighField.baseline
                                }

                            FactTextField {
                                id:         battHighField
                                x:          voltageCol.firstColumnWidth
                                width:      textEditWidth
                                fact:       battHighVolt
                                showUnits:  true
                            }
                        }

                        Row {
                            spacing: ScreenTools.defaultFontPixelWidth

                            QGCLabel {
                                id:                 battLowLabel
                                text:               qsTr("Empty Voltage (per cell)")
                                anchors.baseline:   battLowField.baseline
                            }

                            FactTextField {
                                id:         battLowField
                                x:          voltageCol.firstColumnWidth
                                width:      textEditWidth
                                fact:       battLowVolt
                                showUnits:  true
                            }
                        }
                    } // Column

                    QGCColoredImage {
                        id:                     batteryImage
                        anchors.verticalCenter: voltageCol.verticalCenter
                        x:                      voltageCol.firstColumnWidth + textEditWidth + (ScreenTools.defaultFontPixelWidth * 3)
                        width:                  height * 0.75
                        height:                 voltageCol.height
                        sourceSize.height:      height
                        fillMode:               Image.PreserveAspectFit
                        smooth:                 true
                        color:                  palette.text
                        cache:                  false
                        source:                 getBatteryImage();
                    }

                    Column {
                        anchors.leftMargin:     ScreenTools.defaultFontPixelWidth * 2
                        anchors.left:           batteryImage.right
                        anchors.verticalCenter: voltageCol.verticalCenter
                        spacing:                ScreenTools.defaultFontPixelHeight
                        Row {
                            QGCLabel {
                                width:  ScreenTools.defaultFontPixelWidth * 12
                                text:   qsTr("Battery Max:")
                            }
                            QGCLabel {
                                text:   (battNumCells.value * battHighVolt.value).toFixed(1) + ' V'
                            }
                        }
                        Row {
                            QGCLabel {
                                width:  ScreenTools.defaultFontPixelWidth * 12
                                text:   qsTr("Battery Min:")
                            }
                            QGCLabel {
                                text:   (battNumCells.value * battLowVolt.value).toFixed(1) + ' V'
                            }
                        }
                    }
                } // Rectangle - Battery settings

                QGCLabel {
                    text:           qsTr("ESC PWM Minimum and Maximum Calibration")
                    font.family:    ScreenTools.demiboldFontFamily
                }

                Rectangle {
                    width:  parent.width
                    height: escCalColumn.height + ScreenTools.defaultFontPixelHeight
                    color:  palette.windowShade

                    Column {
                        id :                escCalColumn
                        anchors.margins:    ScreenTools.defaultFontPixelHeight / 2
                        anchors.left:       parent.left
                        anchors.top:        parent.top
                        spacing:            ScreenTools.defaultFontPixelWidth

                        QGCLabel {
                            color:  palette.warningText
                            text:   qsTr("WARNING: Propellers must be removed from vehicle prior to performing ESC calibration.")
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
                    visible:    uavcanEnable !== -1
                }

                QGCLabel {
                    text:           qsTr("UAVCAN Bus Configuration")
                    font.family:    ScreenTools.demiboldFontFamily
                    visible:        showUAVCAN.checked
                }

                Rectangle {
                    width:      parent.width
                    height:     uavCanConfigColumn.height + ScreenTools.defaultFontPixelHeight
                    color:      palette.windowShade
                    visible:    showUAVCAN.checked

                    Column {
                        id:                 uavCanConfigColumn
                        anchors.margins:    ScreenTools.defaultFontPixelHeight / 2
                        anchors.left:       parent.left
                        anchors.top:        parent.top
                        spacing:            ScreenTools.defaultFontPixelWidth

                        FactCheckBox {
                            id:                 uavcanEnabledCheckBox
                            width:              ScreenTools.defaultFontPixelWidth * 20
                            fact:               uavcanEnable
                            checkedValue:       3
                            uncheckedValue:     0
                            text:               qsTr("Enable UAVCAN as the default MAIN output bus (requires autopilot restart)")
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
                    color:      palette.windowShade
                    visible:    showUAVCAN.checked
                    enabled:    uavcanEnabledCheckBox.checked

                    Column {
                        id:                 uavCanEscCalColumn
                        anchors.margins:    ScreenTools.defaultFontPixelHeight / 2
                        anchors.left:       parent.left
                        anchors.top:        parent.top
                        spacing:            ScreenTools.defaultFontPixelWidth

                        QGCLabel {
                            color:  palette.warningText
                            text:   qsTr("WARNING: Propellers must be removed from vehicle prior to performing UAVCAN ESC configuration.")
                        }

                        QGCLabel {
                            text: qsTr("ESC parameters will only be accessible in the editor after assignment.")
                        }

                        QGCLabel {
                            text: qsTr("Start the process, then turn each motor into its turn direction, in the order of their motor indices.")
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
                    color:      palette.windowShade
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
        } // QGCFlickable
    } // QGCViewPanel
} // QGCView

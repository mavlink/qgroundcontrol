/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

import QtQuick          2.5
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2

import QGroundControl               1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0

QGCView {
    id:         qgcView
    viewPanel:  panel

    QGCPalette { id: palette; colorGroupEnabled: panel.enabled }

    property int                _firstColumn:   ScreenTools.defaultFontPixelWidth * 20
    property int                _secondColumn:  ScreenTools.defaultFontPixelWidth * 12
    readonly property string    dialogTitle:    "controller WiFi Bridge"

    ESP8266ComponentController {
        id:             controller
        factPanel:      panel
    }

    property Fact wifiChannel:  controller.getParameterFact(controller.componentID, "WIFI_CHANNEL")
    property Fact hostPort:     controller.getParameterFact(controller.componentID, "WIFI_UDP_HPORT")
    property Fact clientPort:   controller.getParameterFact(controller.componentID, "WIFI_UDP_CPORT")

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        Flickable {
            anchors.fill:       parent
            clip:               true
            contentHeight:      innerColumn.height
            contentWidth:       panel.width
            boundsBehavior:     Flickable.StopAtBounds
            flickableDirection: Flickable.VerticalFlick

            Column {
                id:             innerColumn
                width:          panel.width
                spacing:        ScreenTools.defaultFontPixelHeight * 0.5

                QGCLabel {
                    text: "WiFi Bridge Settings"
                    font.weight: Font.DemiBold
                }

                Rectangle {
                    width:  parent.width
                    height: wifiCol.height + (ScreenTools.defaultFontPixelHeight * 2)
                    color:  palette.windowShade
                    Column {
                        id:                 wifiCol
                        anchors.margins:    ScreenTools.defaultFontPixelHeight / 2
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left:       parent.left
                        spacing:            ScreenTools.defaultFontPixelHeight / 2
                        Row {
                            spacing: ScreenTools.defaultFontPixelWidth
                            QGCLabel {
                                text:           "WiFi Channel"
                                width:          _firstColumn
                                anchors.baseline: channelField.baseline
                            }
                            QGCComboBox {
                                id:             channelField
                                width:          _secondColumn
                                model:          controller.wifiChannels
                                currentIndex:   wifiChannel ? wifiChannel.value - 1 : 0
                                onActivated: {
                                    wifiChannel.value = index + 1
                                }
                            }
                        }
                        Row {
                            spacing: ScreenTools.defaultFontPixelWidth
                            QGCLabel {
                                text:           "WiFi SSID"
                                width:          _firstColumn
                                anchors.baseline: ssidField.baseline
                                }
                            QGCTextField {
                                id:             ssidField
                                width:          _secondColumn
                                text:           controller.wifiSSID
                                maximumLength:  16
                                onEditingFinished: {
                                    controller.wifiSSID = text
                                }
                            }
                        }
                        Row {
                            spacing: ScreenTools.defaultFontPixelWidth
                            QGCLabel {
                                text:           "WiFi Password"
                                width:          _firstColumn
                                anchors.baseline: passwordField.baseline
                                }
                            QGCTextField {
                                id:             passwordField
                                width:          _secondColumn
                                text:           controller.wifiPassword
                                maximumLength:  16
                                onEditingFinished: {
                                    controller.wifiPassword = text
                                }
                            }
                        }
                        Row {
                            spacing: ScreenTools.defaultFontPixelWidth
                            QGCLabel {
                                text:           "UART Baud Rate"
                                width:          _firstColumn
                                anchors.baseline:   baudField.baseline
                            }
                            QGCComboBox {
                                id:             baudField
                                width:          _secondColumn
                                model:          controller.baudRates
                                currentIndex:   controller.baudIndex
                                onActivated: {
                                    controller.baudIndex = index
                                }
                            }
                        }
                        Row {
                            spacing: ScreenTools.defaultFontPixelWidth
                            QGCLabel {
                                text:           "QGC UDP Port"
                                width:          _firstColumn
                                anchors.baseline: qgcportField.baseline
                            }
                            QGCTextField {
                                id:             qgcportField
                                width:          _secondColumn
                                text:           hostPort ? hostPort.valueString : ""
                                validator:      IntValidator {bottom: 1024; top: 65535;}
                                inputMethodHints: Qt.ImhDigitsOnly
                                onEditingFinished: {
                                    hostPort.value = text
                                }
                            }
                        }
                    }
                }
                Row {
                    spacing: ScreenTools.defaultFontPixelWidth * 1.5
                    QGCButton {
                        text:   "Restore Defaults"
                        width:  ScreenTools.defaultFontPixelWidth * 16
                        onClicked: {
                            controller.restoreDefaults()
                        }
                    }
                    QGCButton {
                        text:           "Restart WiFi Bridge"
                        enabled:        !controller.busy
                        width:          ScreenTools.defaultFontPixelWidth * 16
                        onClicked: {
                            rebootDialog.visible = true
                        }
                        MessageDialog {
                            id:         rebootDialog
                            visible:    false
                            icon:       StandardIcon.Warning
                            standardButtons: StandardButton.Yes | StandardButton.No
                            title:      "Reboot WiFi Bridge"
                            text:       "This will restart the WiFi Bridge so the settings you've changed can take effect. Note that you may have to change your computer WiFi settings and QGroundControl link settings to match these changes. Are you sure you want to restart it?"
                            onYes: {
                                controller.reboot()
                                rebootDialog.visible = false
                            }
                            onNo: {
                                rebootDialog.visible = false
                            }
                        }
                    }
                }
            }
        }
    }
}

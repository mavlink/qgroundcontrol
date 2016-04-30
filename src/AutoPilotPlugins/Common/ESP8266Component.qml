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
import QtQuick.Layouts  1.2

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

    property real _margins:         ScreenTools.defaultFontPixelHeight
    property real _middleRowWidth:  ScreenTools.defaultFontPixelWidth * 18
    property real _editFieldWidth:  ScreenTools.defaultFontPixelWidth * 18
    property real _labelWidth:      ScreenTools.defaultFontPixelWidth * 10
    property real _statusWidth:     ScreenTools.defaultFontPixelWidth * 5

    readonly property string    dialogTitle:    qsTr("controller WiFi Bridge")
    property int                stStatus:       XMLHttpRequest.UNSENT
    property int                stErrorCount:   0
    property bool               stResetCounters:false

    ESP8266ComponentController {
        id:             controller
        factPanel:      panel
    }

    Timer {
        id: timer
    }

    function thisThingHasNoNumberLocaleSupport(n) {
        return n.toString().replace(/\B(?=(\d{3})+(?!\d))/g, ",").replace(",,", ",")
    }

    function updateStatus() {
        timer.stop()
        var req = new XMLHttpRequest;
        var url = "http://"
        url += controller.wifiIPAddress
        url += "/status.json"
        if(stResetCounters) {
            url = url + "?r=1"
            stResetCounters = false
        }
        req.open("GET", url);
        req.onreadystatechange = function() {
            stStatus = req.readyState;
            if (stStatus === XMLHttpRequest.DONE) {
                var objectArray = JSON.parse(req.responseText);
                if (objectArray.errors !== undefined) {
                    console.log(qsTr("Error fetching WiFi Bridge Status: %1").arg(objectArray.errors[0].message))
                    stErrorCount = stErrorCount + 1
                    if(stErrorCount < 2)
                        timer.start()
                } else {
                    //-- This should work but it doesn't
                    //   var n = 34523453.345
                    //   n.toLocaleString()
                    //   "34,523,453.345"
                    vpackets.text   = thisThingHasNoNumberLocaleSupport(objectArray["vpackets"])
                    vsent.text      = thisThingHasNoNumberLocaleSupport(objectArray["vsent"])
                    vlost.text      = thisThingHasNoNumberLocaleSupport(objectArray["vlost"])
                    gpackets.text   = thisThingHasNoNumberLocaleSupport(objectArray["gpackets"])
                    gsent.text      = thisThingHasNoNumberLocaleSupport(objectArray["gsent"])
                    glost.text      = thisThingHasNoNumberLocaleSupport(objectArray["glost"])
                    stErrorCount    = 0
                    timer.start()
                }
            }
        }
        req.send()
    }

    Component.onCompleted: {
        timer.interval = 1000
        timer.repeat = true
        timer.triggered.connect(updateStatus)
        timer.start()
    }

    property Fact wifiMode:     controller.getParameterFact(controller.componentID, "WIFI_MODE",      false) //-- Don't bitch about missing as this is new
    property Fact wifiChannel:  controller.getParameterFact(controller.componentID, "WIFI_CHANNEL")
    property Fact hostPort:     controller.getParameterFact(controller.componentID, "WIFI_UDP_HPORT")
    property Fact clientPort:   controller.getParameterFact(controller.componentID, "WIFI_UDP_CPORT")

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        Flickable {
            clip:                                       true
            anchors.fill:                               parent
            contentHeight:                              mainCol.height
            flickableDirection:                         Flickable.VerticalFlick
            Column {
                id:                                     mainCol
                spacing:                                _margins
                anchors.horizontalCenter:               parent.horizontalCenter
                Item { width: 1; height: _margins * 0.5; }
                QGCLabel {
                    text:                               qsTr("ESP WiFi Bridge Settings")
                    font.weight:                        Font.DemiBold
                }
                Rectangle {
                    color:                              palette.windowShade
                    width:                              statusLayout.width  + _margins * 4
                    height:                             settingsRow.height  + _margins * 2
                    Row {
                        id:                             settingsRow
                        spacing:                        _margins
                        anchors.centerIn:               parent
                        Item { width: _margins * 0.5; height: 1; }
                        QGCColoredImage {
                            color:                      palette.text
                            height:                     ScreenTools.defaultFontPixelWidth * 10
                            width:                      ScreenTools.defaultFontPixelWidth * 12
                            mipmap:                     true
                            fillMode:                   Image.PreserveAspectFit
                            source:                     wifiMode ? (wifiMode.value === 0 ? "/qmlimages/APMode.svg" : "/qmlimages/StationMode.svg") : "/qmlimages/APMode.svg"
                            anchors.verticalCenter:     parent.verticalCenter
                        }
                        Item { width: _margins * 0.5; height: 1; }
                        Column {
                            spacing:                        _margins * 0.5
                            anchors.verticalCenter:         parent.verticalCenter
                            Row {
                                visible:                    wifiMode
                                QGCLabel {
                                    text:                   qsTr("WiFi Mode")
                                    width:                  _middleRowWidth
                                    anchors.baseline:       modeField.baseline
                                }
                                QGCComboBox {
                                    id:                     modeField
                                    width:                  _editFieldWidth
                                    model:                  ["Access Point Mode", "Station Mode"]
                                    currentIndex:           wifiMode ? wifiMode.value : 0
                                    onActivated: {
                                        wifiMode.value = index
                                    }
                                }
                            }
                            Row {
                                QGCLabel {
                                    text:                   qsTr("WiFi Channel")
                                    width:                  _middleRowWidth
                                    anchors.baseline:       channelField.baseline
                                }
                                QGCComboBox {
                                    id:                     channelField
                                    width:                  _editFieldWidth
                                    enabled:                wifiMode && wifiMode.value === 0
                                    model:                  controller.wifiChannels
                                    currentIndex:           wifiChannel ? wifiChannel.value - 1 : 0
                                    onActivated: {
                                        wifiChannel.value = index + 1
                                    }
                                }
                            }
                            Row {
                                QGCLabel {
                                    text:                   qsTr("WiFi AP SSID")
                                    width:                  _middleRowWidth
                                    anchors.baseline:       ssidField.baseline
                                }
                                QGCTextField {
                                    id:                     ssidField
                                    width:                  _editFieldWidth
                                    text:                   controller.wifiSSID
                                    maximumLength:          16
                                    onEditingFinished: {
                                        controller.wifiSSID = text
                                    }
                                }
                            }
                            Row {
                                QGCLabel {
                                    text:                   qsTr("WiFi AP Password")
                                    width:                  _middleRowWidth
                                    anchors.baseline:       passwordField.baseline
                                }
                                QGCTextField {
                                    id:                     passwordField
                                    width:                  _editFieldWidth
                                    text:                   controller.wifiPassword
                                    maximumLength:          16
                                    onEditingFinished: {
                                        controller.wifiPassword = text
                                    }
                                }
                            }
                            Row {
                                QGCLabel {
                                    text:                   qsTr("WiFi STA SSID")
                                    width:                  _middleRowWidth
                                    anchors.baseline:       stassidField.baseline
                                }
                                QGCTextField {
                                    id:                     stassidField
                                    width:                  _editFieldWidth
                                    text:                   controller.wifiSSIDSta
                                    maximumLength:          16
                                    enabled:                wifiMode && wifiMode.value === 1
                                    onEditingFinished: {
                                        controller.wifiSSIDSta = text
                                    }
                                }
                            }
                            Row {
                                QGCLabel {
                                    text:                   qsTr("WiFi STA Password")
                                    width:                  _middleRowWidth
                                    anchors.baseline:       passwordStaField.baseline
                                }
                                QGCTextField {
                                    id:                     passwordStaField
                                    width:                  _editFieldWidth
                                    text:                   controller.wifiPasswordSta
                                    maximumLength:          16
                                    enabled:                wifiMode && wifiMode.value === 1
                                    onEditingFinished: {
                                        controller.wifiPasswordSta = text
                                    }
                                }
                            }
                            Row {
                                QGCLabel {
                                    text:                   qsTr("UART Baud Rate")
                                    width:                  _middleRowWidth
                                    anchors.baseline:       baudField.baseline
                                }
                                QGCComboBox {
                                    id:                     baudField
                                    width:                  _editFieldWidth
                                    model:                  controller.baudRates
                                    currentIndex:           controller.baudIndex
                                    onActivated: {
                                        controller.baudIndex = index
                                    }
                                }
                            }
                            Row {
                                QGCLabel {
                                    text:                   qsTr("QGC UDP Port")
                                    width:                  _middleRowWidth
                                    anchors.baseline:       qgcportField.baseline
                                }
                                QGCTextField {
                                    id:                     qgcportField
                                    width:                  _editFieldWidth
                                    text:                   hostPort ? hostPort.valueString : ""
                                    validator:              IntValidator {bottom: 1024; top: 65535;}
                                    inputMethodHints:       Qt.ImhDigitsOnly
                                    onEditingFinished: {
                                        hostPort.value = text
                                    }
                                }
                            }
                        }
                    }
                }
                QGCLabel {
                    text:                               qsTr("ESP WiFi Bridge Status")
                    font.weight:                        Font.DemiBold
                }
                Rectangle {
                    color:                              palette.windowShade
                    width:                              statusLayout.width  + _margins * 4
                    height:                             statusLayout.height + _margins * 2
                    GridLayout {
                       id:                              statusLayout
                       columns:                         3
                       columnSpacing:                   _margins * 2
                       anchors.centerIn:                parent
                       QGCLabel {
                           text:                        qsTr("Bridge/Vehicle Link")
                           Layout.alignment:            Qt.AlignHCenter
                       }
                       QGCLabel {
                           text:                        qsTr("Bridge/QGC Link")
                           Layout.alignment:            Qt.AlignHCenter
                       }
                       QGCLabel {
                           text:                        qsTr("QGC/Bridge Link")
                           Layout.alignment:            Qt.AlignHCenter
                       }
                       Row {
                           spacing:                     _margins
                           QGCLabel {
                               text:                    qsTr("Messages Received")
                               font.pixelSize:          ScreenTools.smallFontPixelSize
                               width:                   _labelWidth
                           }
                           QGCLabel {
                               id:                      vpackets
                               font.pixelSize:          ScreenTools.smallFontPixelSize
                               width:                   _statusWidth
                               horizontalAlignment:     Text.AlignRight
                               font.family:             "Monospace"
                           }
                       }
                       Row {
                           spacing:                     _margins
                           QGCLabel {
                               font.pixelSize:          ScreenTools.smallFontPixelSize
                               text:                    qsTr("Messages Received")
                               width:                   _labelWidth
                           }
                           QGCLabel {
                               id:                      gpackets
                               font.pixelSize:          ScreenTools.smallFontPixelSize
                               width:                   _statusWidth
                               horizontalAlignment:     Text.AlignRight
                               font.family:             "Monospace"
                           }
                       }
                       Row {
                           spacing:                     _margins
                           QGCLabel {
                               font.pixelSize:          ScreenTools.smallFontPixelSize
                               text:                    qsTr("Messages Received")
                               width:                   _labelWidth
                           }
                           QGCLabel {
                               font.pixelSize:          ScreenTools.smallFontPixelSize
                               text:                    controller.vehicle ? thisThingHasNoNumberLocaleSupport(controller.vehicle.messagesReceived) : 0
                               width:                   _statusWidth
                               horizontalAlignment:     Text.AlignRight
                               font.family:             "Monospace"
                           }
                       }
                       Row {
                           spacing:                     _margins
                           QGCLabel {
                               text:                    qsTr("Messages Lost")
                               font.pixelSize:          ScreenTools.smallFontPixelSize
                               width:                   _labelWidth
                           }
                           QGCLabel {
                               id:                      vlost
                               width:                   _statusWidth
                               horizontalAlignment:     Text.AlignRight
                               font.pixelSize:          ScreenTools.smallFontPixelSize
                               font.family:             "Monospace"
                           }
                       }
                       Row {
                           spacing:                     _margins
                           QGCLabel {
                               text:                    qsTr("Messages Lost")
                               font.pixelSize:          ScreenTools.smallFontPixelSize
                               width:                   _labelWidth
                           }
                           QGCLabel {
                               id:                      glost
                               width:                   _statusWidth
                               horizontalAlignment:     Text.AlignRight
                               font.pixelSize:          ScreenTools.smallFontPixelSize
                               font.family:             "Monospace"
                           }
                       }
                       Row {
                           spacing:                     _margins
                           QGCLabel {
                               text:                    qsTr("Messages Lost")
                               font.pixelSize:          ScreenTools.smallFontPixelSize
                               width:                   _labelWidth
                           }
                           QGCLabel {
                               text:                    controller.vehicle ? thisThingHasNoNumberLocaleSupport(controller.vehicle.messagesLost) : 0
                               width:                   _statusWidth
                               horizontalAlignment:     Text.AlignRight
                               font.pixelSize:          ScreenTools.smallFontPixelSize
                               font.family:             "Monospace"
                           }
                       }
                       Row {
                           spacing:                     _margins
                           QGCLabel {
                               text:                    qsTr("Messages Sent")
                               font.pixelSize:          ScreenTools.smallFontPixelSize
                               width:                   _labelWidth
                           }
                           QGCLabel {
                               id:                      vsent
                               width:                   _statusWidth
                               horizontalAlignment:     Text.AlignRight
                               font.pixelSize:          ScreenTools.smallFontPixelSize
                               font.family:             "Monospace"
                           }
                       }
                       Row {
                           spacing:                     _margins
                           QGCLabel {
                               text:                    qsTr("Messages Sent")
                               font.pixelSize:          ScreenTools.smallFontPixelSize
                               width:                   _labelWidth
                           }
                           QGCLabel {
                               id:                      gsent
                               width:                   _statusWidth
                               horizontalAlignment:     Text.AlignRight
                               font.pixelSize:          ScreenTools.smallFontPixelSize
                               font.family:             "Monospace"
                           }
                       }
                       Row {
                           spacing:                     _margins
                           QGCLabel {
                               text:                    qsTr("Messages Sent")
                               font.pixelSize:          ScreenTools.smallFontPixelSize
                               width:                   _labelWidth
                           }
                           QGCLabel {
                               text:                    controller.vehicle ? thisThingHasNoNumberLocaleSupport(controller.vehicle.messagesSent) : 0
                               width:                   _statusWidth
                               horizontalAlignment:     Text.AlignRight
                               font.pixelSize:          ScreenTools.smallFontPixelSize
                               font.family:             "Monospace"
                           }
                       }
                    }
                }
                Row {
                    spacing:                            _margins
                    anchors.horizontalCenter:           parent.horizontalCenter
                    QGCButton {
                        text:                           qsTr("Restore Defaults")
                        width:                          ScreenTools.defaultFontPixelWidth * 16
                        onClicked: {
                            controller.restoreDefaults()
                        }
                    }
                    QGCButton {
                        text:                           qsTr("Restart WiFi Bridge")
                        enabled:                        !controller.busy
                        width:                          ScreenTools.defaultFontPixelWidth * 16
                        onClicked: {
                            rebootDialog.visible = true
                        }
                        MessageDialog {
                            id:         rebootDialog
                            visible:    false
                            icon:       StandardIcon.Warning
                            standardButtons: StandardButton.Yes | StandardButton.No
                            title:      qsTr("Reboot WiFi Bridge")
                            text:       qsTr("This will restart the WiFi Bridge so the settings you've changed can take effect. Note that you may have to change your computer WiFi settings and QGroundControl link settings to match these changes. Are you sure you want to restart it?")
                            onYes: {
                                controller.reboot()
                                rebootDialog.visible = false
                            }
                            onNo: {
                                rebootDialog.visible = false
                            }
                        }
                    }
                    QGCButton {
                        text:                           qsTr("Reset Counters")
                        width:                          ScreenTools.defaultFontPixelWidth * 16
                        onClicked: {
                            stResetCounters = true;
                            updateStatus()
                            if(controller.vehicle)
                                controller.vehicle.resetCounters()
                        }
                    }
                }
            }
        }
    }
}

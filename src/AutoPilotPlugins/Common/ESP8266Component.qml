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

    property int                _firstColumn:   ScreenTools.defaultFontPixelWidth * 20
    property int                _secondColumn:  ScreenTools.defaultFontPixelWidth * 12
    readonly property string    dialogTitle:    qsTr("controller WiFi Bridge")
    property int                stStatus:       XMLHttpRequest.UNSENT
    property int                stErrorCount:   0
    property bool               stEnabled:      false
    property bool               stResetCounters: false

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
        var url = "http://192.168.4.1/status.json"
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
                    if(stErrorCount < 2 && stEnabled)
                        timer.start()
                } else {
                    if(stEnabled) {
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
                        wifiStatus.visible = true
                        timer.start()
                    }
                }
            }
        }
        req.send()
    }

    Component.onCompleted: {
        timer.interval = 1000
        timer.repeat = true
        timer.triggered.connect(updateStatus)
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
                    text: qsTr("WiFi Bridge Settings")
                    font.weight: Font.DemiBold
                }

                Rectangle {
                    width:  parent.width
                    height: wifiStatus.visible ? Math.max(wifiCol.height, wifiStatus.height) + (ScreenTools.defaultFontPixelHeight * 2) : wifiCol.height + (ScreenTools.defaultFontPixelHeight * 2)
                    color:  palette.windowShade
                    Row {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: ScreenTools.defaultFontPixelWidth
                        Rectangle {
                            height:     parent.height
                            width:      1
                            color:      palette.window
                        }
                        Column {
                            id:                 wifiCol
                            anchors.verticalCenter: parent.verticalCenter
                            spacing:            ScreenTools.defaultFontPixelHeight / 2
                            Row {
                                spacing: ScreenTools.defaultFontPixelWidth
                                QGCLabel {
                                    text:           qsTr("WiFi Channel")
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
                                    text:           qsTr("WiFi SSID")
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
                                    text:           qsTr("WiFi Password")
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
                                    text:           qsTr("UART Baud Rate")
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
                                    text:           qsTr("QGC UDP Port")
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
                        Rectangle {
                            height:         parent.height
                            width:          1
                            color:          palette.text
                            visible:        wifiStatus.visible
                        }
                        Column {
                            id:                 wifiStatus
                            anchors.margins:    ScreenTools.defaultFontPixelHeight / 2
                            spacing:            ScreenTools.defaultFontPixelHeight / 2
                            visible:            false
                            QGCLabel {
                                text:           qsTr("Bridge/Vehicle Link")
                                font.weight:    Font.DemiBold
                            }
                            Row {
                                spacing: ScreenTools.defaultFontPixelWidth
                                QGCLabel {
                                    text:       qsTr("Messages Received")
                                    width:      _firstColumn
                                }
                                QGCLabel {
                                    id:         vpackets
                                }
                            }
                            Row {
                                spacing: ScreenTools.defaultFontPixelWidth
                                QGCLabel {
                                    text:       qsTr("Messages Lost")
                                    width:      _firstColumn
                                }
                                QGCLabel {
                                    id:         vlost
                                }
                            }
                            Row {
                                spacing: ScreenTools.defaultFontPixelWidth
                                QGCLabel {
                                    text:       qsTr("Messages Sent")
                                    width:      _firstColumn
                                }
                                QGCLabel {
                                    id:         vsent
                                }
                            }
                            Rectangle {
                                height:         1
                                width:          parent.width
                                color:          palette.text
                            }
                            QGCLabel {
                                text:           qsTr("Bridge/QGC Link")
                                font.weight:    Font.DemiBold
                            }
                            Row {
                                spacing: ScreenTools.defaultFontPixelWidth
                                QGCLabel {
                                    text:       qsTr("Messages Received")
                                    width:      _firstColumn
                                }
                                QGCLabel {
                                    id:         gpackets
                                }
                            }
                            Row {
                                spacing: ScreenTools.defaultFontPixelWidth
                                QGCLabel {
                                    text:       qsTr("Messages Lost")
                                    width:      _firstColumn
                                }
                                QGCLabel {
                                    id:         glost
                                }
                            }
                            Row {
                                spacing: ScreenTools.defaultFontPixelWidth
                                QGCLabel {
                                    text:       qsTr("Messages Sent")
                                    width:      _firstColumn
                                }
                                QGCLabel {
                                    id:         gsent
                                }
                            }
                            Rectangle {
                                height:         1
                                width:          parent.width
                                color:          palette.text
                            }
                            QGCLabel {
                                text:           qsTr("QGC/Bridge Link")
                                font.weight:    Font.DemiBold
                            }
                            Row {
                                spacing: ScreenTools.defaultFontPixelWidth
                                QGCLabel {
                                    text:       qsTr("Messages Received")
                                    width:      _firstColumn
                                }
                                QGCLabel {
                                    text:       controller.vehicle ? thisThingHasNoNumberLocaleSupport(controller.vehicle.messagesReceived) : 0
                                }
                            }
                            Row {
                                spacing: ScreenTools.defaultFontPixelWidth
                                QGCLabel {
                                    text:       qsTr("Messages Lost")
                                    width:      _firstColumn
                                }
                                QGCLabel {
                                    text:       controller.vehicle ? thisThingHasNoNumberLocaleSupport(controller.vehicle.messagesLost) : 0
                                }
                            }
                            Row {
                                spacing: ScreenTools.defaultFontPixelWidth
                                QGCLabel {
                                    text:       qsTr("Messages Sent")
                                    width:      _firstColumn
                                }
                                QGCLabel {
                                    text:       controller.vehicle ? thisThingHasNoNumberLocaleSupport(controller.vehicle.messagesSent) : 0
                                }
                            }
                        }
                    }
                }
                Row {
                    spacing: ScreenTools.defaultFontPixelWidth * 1.5
                    QGCButton {
                        text:   qsTr("Restore Defaults")
                        width:  ScreenTools.defaultFontPixelWidth * 16
                        onClicked: {
                            controller.restoreDefaults()
                        }
                    }
                    QGCButton {
                        text:           qsTr("Restart WiFi Bridge")
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
                        text:   stEnabled ? qsTr("Hide Status") : qsTr("Show Status")
                        width:  ScreenTools.defaultFontPixelWidth * 16
                        onClicked: {
                            stEnabled = !stEnabled
                            if(stEnabled)
                                updateStatus()
                            else {
                                wifiStatus.visible = false
                                timer.stop()
                            }
                        }
                    }
                    QGCButton {
                        text:       qsTr("Reset Counters")
                        visible:    stEnabled
                        enabled:    stEnabled
                        width:      ScreenTools.defaultFontPixelWidth * 16
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

/****************************************************************************
 *
 *   (c) 2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtGraphicalEffects       1.0
import QtMultimedia             5.5
import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
import QtQuick.Layouts          1.2
import QtLocation               5.3
import QtPositioning            5.3

import QGroundControl                       1.0
import QGroundControl.Controllers           1.0
import QGroundControl.Controls              1.0
import QGroundControl.FactControls          1.0
import QGroundControl.FactSystem            1.0
import QGroundControl.Palette               1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.SettingsManager       1.0

Rectangle {
    id:                 _root
    color:              qgcPal.window
    anchors.fill:       parent
    anchors.margins:    ScreenTools.defaultFontPixelWidth

    property real _labelWidth:                  ScreenTools.defaultFontPixelWidth * 26
    property real _valueWidth:                  ScreenTools.defaultFontPixelWidth * 20
    property real _panelWidth:                  _root.width * _internalWidthRatio
    property Fact _microhardEnabledFact:        QGroundControl.settingsManager.appSettings.enableMicrohard
    property bool _microhardEnabled:            _microhardEnabledFact ? _microhardEnabledFact.rawValue : false

    readonly property real _internalWidthRatio:          0.8

        QGCFlickable {
            clip:               true
            anchors.fill:       parent
            contentHeight:      settingsColumn.height
            contentWidth:       settingsColumn.width
            Column {
                id:                 settingsColumn
            width:              _root.width
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                anchors.margins:    ScreenTools.defaultFontPixelWidth
                //-----------------------------------------------------------------
                //-- General
                Item {
                    width:                      _panelWidth
                    height:                     generalLabel.height
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    QGCLabel {
                        id:             generalLabel
                        text:           qsTr("General")
                        font.family:    ScreenTools.demiboldFontFamily
                    }
                }
                Rectangle {
                    height:                     generalRow.height + (ScreenTools.defaultFontPixelHeight * 2)
                    width:                      _panelWidth
                    color:                      qgcPal.windowShade
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    Row {
                        id:                 generalRow
                        spacing:            ScreenTools.defaultFontPixelWidth * 4
                        anchors.centerIn:   parent
                        Column {
                            spacing:        ScreenTools.defaultFontPixelWidth
                            FactCheckBox {
                                text:       qsTr("Enable Microhard")
                                fact:       _microhardEnabledFact
                                enabled:    true
                                visible:    _microhardEnabledFact ? _microhardEnabledFact.visible : false
                            }
                        }
                    }
                }
                //-----------------------------------------------------------------
                //-- Connection Status
                Item {
                    width:                      _panelWidth
                    height:                     statusLabel.height
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    visible:                    _microhardEnabled
                    QGCLabel {
                        id:                     statusLabel
                        text:                   qsTr("Connection Status")
                        font.family:            ScreenTools.demiboldFontFamily
                    }
                }
                Rectangle {
                    height:                     statusCol.height + (ScreenTools.defaultFontPixelHeight * 2)
                    width:                      _panelWidth
                    color:                      qgcPal.windowShade
                    visible:                    _microhardEnabled
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    Column {
                        id:                     statusCol
                        spacing:                ScreenTools.defaultFontPixelHeight * 0.5
                        width:                  parent.width
                        anchors.centerIn:       parent
                        GridLayout {
                            anchors.margins:    ScreenTools.defaultFontPixelHeight
                            columnSpacing:      ScreenTools.defaultFontPixelWidth * 2
                            anchors.horizontalCenter: parent.horizontalCenter
                            columns: 2
                            QGCLabel {
                                text:           qsTr("Ground Unit:")
                                Layout.minimumWidth: _labelWidth
                            }
                            QGCLabel {
                                function getStatus(status) {
                                    if (status === 1)
                                        return qsTr("Connected");
                                    else if (status === -1)
                                        return qsTr("Login Error")
                                    else
                                        return qsTr("Not Connected")
                                }
                                text:           getStatus(QGroundControl.microhardManager.connected)
                                color:          QGroundControl.microhardManager.connected === 1 ? qgcPal.colorGreen : qgcPal.colorRed
                                Layout.minimumWidth: _valueWidth
                            }
                            QGCLabel {
                                text:           qsTr("Air Unit:")
                            }
                            QGCLabel {
                                function getStatus(status) {
                                    if (status === 1)
                                        return qsTr("Connected");
                                    else if (status === -1)
                                        return qsTr("Login Error")
                                    else
                                        return qsTr("Not Connected")
                                }
                                text:           getStatus(QGroundControl.microhardManager.linkConnected)
                                color:          QGroundControl.microhardManager.linkConnected === 1 ? qgcPal.colorGreen : qgcPal.colorRed
                            }
                            QGCLabel {
                                text:           qsTr("Uplink RSSI:")
                            }
                            QGCLabel {
                                text:           QGroundControl.microhardManager.linkConnected && QGroundControl.microhardManager.uplinkRSSI < 0 ? QGroundControl.microhardManager.uplinkRSSI : ""
                            }
                            QGCLabel {
                                text:           qsTr("Downlink RSSI:")
                            }
                            QGCLabel {
                                text:           QGroundControl.microhardManager.linkConnected && QGroundControl.microhardManager.downlinkRSSI < 0 ? QGroundControl.microhardManager.downlinkRSSI : ""
                            }
                        }
                    }
                }
                //-----------------------------------------------------------------
                //-- IP Settings
                Item {
                    width:                      _panelWidth
                    height:                     ipSettingsLabel.height
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    visible:                    _microhardEnabled
                    QGCLabel {
                        id:                     ipSettingsLabel
                        text:                   qsTr("Network Settings")
                        font.family:            ScreenTools.demiboldFontFamily
                    }
                }
                Rectangle {
                    height:                     ipSettingsCol.height + (ScreenTools.defaultFontPixelHeight * 2)
                    width:                      _panelWidth
                    color:                      qgcPal.windowShade
                    visible:                    _microhardEnabled
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    Column {
                        id:                     ipSettingsCol
                        spacing:                ScreenTools.defaultFontPixelHeight * 0.5
                        width:                  parent.width
                        anchors.centerIn:       parent
                        GridLayout {
                            anchors.margins:    ScreenTools.defaultFontPixelHeight
                            columnSpacing:      ScreenTools.defaultFontPixelWidth * 2
                            anchors.horizontalCenter: parent.horizontalCenter
                            columns: 2
                            QGCLabel {
                                text:           qsTr("Local IP Address:")
                                Layout.minimumWidth: _labelWidth
                            }
                            QGCTextField {
                                id:             localIP
                                text:           QGroundControl.microhardManager.localIPAddr
                                enabled:        true
                                inputMethodHints:    Qt.ImhFormattedNumbersOnly
                                Layout.minimumWidth: _valueWidth
                            }
                            QGCLabel {
                                text:           qsTr("Remote IP Address:")
                            }
                            QGCTextField {
                                id:             remoteIP
                                text:           QGroundControl.microhardManager.remoteIPAddr
                                enabled:        true
                                inputMethodHints:    Qt.ImhFormattedNumbersOnly
                                Layout.minimumWidth: _valueWidth
                            }
                            QGCLabel {
                                text:           qsTr("Network Mask:")
                            }
                            QGCTextField {
                                id:             netMask
                                text:           QGroundControl.microhardManager.netMask
                                enabled:        true
                                inputMethodHints:    Qt.ImhFormattedNumbersOnly
                                Layout.minimumWidth: _valueWidth
                            }
                            QGCLabel {
                                text:           qsTr("Configuration User Name:")
                            }
                            QGCTextField {
                                id:             configUserName
                                text:           QGroundControl.microhardManager.configUserName
                                enabled:        true
                                Layout.minimumWidth: _valueWidth
                            }
                            QGCLabel {
                                text:           qsTr("Configuration Password:")
                            }
                            QGCTextField {
                                id:             configPassword
                                text:           QGroundControl.microhardManager.configPassword
                                enabled:        true
                                echoMode:       TextInput.Password
                                Layout.minimumWidth: _valueWidth
                            }
                            QGCLabel {
                                text:           qsTr("Encryption key:")
                            }
                            QGCTextField {
                                id:             encryptionKey
                                text:           QGroundControl.microhardManager.encryptionKey
                                enabled:        true
                                echoMode:       TextInput.Password
                                Layout.minimumWidth: _valueWidth
                            }
                        }
                        Item {
                            width:  1
                            height: ScreenTools.defaultFontPixelHeight
                        }
                        QGCButton {
                            function validateIPaddress(ipaddress) {
                                if (/^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/.test(ipaddress))
                                    return true
                                return false
                            }
                            function testEnabled() {
                                if(localIP.text          === QGroundControl.microhardManager.localIPAddr &&
                                    remoteIP.text        === QGroundControl.microhardManager.remoteIPAddr &&
                                    netMask.text         === QGroundControl.microhardManager.netMask &&
                                    configUserName.text  === QGroundControl.microhardManager.configUserName &&
                                    configPassword.text  === QGroundControl.microhardManager.configPassword &&
                                    encryptionKey.text   === QGroundControl.microhardManager.encryptionKey)
                                    return false
                                if(!validateIPaddress(localIP.text))  return false
                                if(!validateIPaddress(remoteIP.text)) return false
                                if(!validateIPaddress(netMask.text))  return false
                                return true
                            }
                            enabled:            testEnabled()
                            text:               qsTr("Apply")
                            anchors.horizontalCenter:   parent.horizontalCenter
                            onClicked: {
                                QGroundControl.microhardManager.setIPSettings(localIP.text, remoteIP.text, netMask.text, configUserName.text, configPassword.text, encryptionKey.text)
                            }

                        }
                    }
                }
            }
        }
    }

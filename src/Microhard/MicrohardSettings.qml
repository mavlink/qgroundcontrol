/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

QGCView {
    id:                 _qgcView
    viewPanel:          panel
    color:              qgcPal.window
    anchors.fill:       parent
    anchors.margins:    ScreenTools.defaultFontPixelWidth

    property real _labelWidth:                  ScreenTools.defaultFontPixelWidth * 26
    property real _valueWidth:                  ScreenTools.defaultFontPixelWidth * 20
    property real _panelWidth:                  _qgcView.width * _internalWidthRatio
    property Fact _microhardEnabledFact:        QGroundControl.settingsManager.appSettings.enableMicrohard
    property Fact _microhardVideoEnabledFact:   QGroundControl.settingsManager.appSettings.enableMicrohardVideo
    property bool _microhardEnabled:            _microhardEnabledFact.rawValue

    readonly property real _internalWidthRatio:          0.8

    QGCPalette { id: qgcPal }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent
        QGCFlickable {
            clip:               true
            anchors.fill:       parent
            contentHeight:      settingsColumn.height
            contentWidth:       settingsColumn.width
            Column {
                id:                 settingsColumn
                width:              _qgcView.width
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                anchors.margins:    ScreenTools.defaultFontPixelWidth
                QGCLabel {
                    text:           qsTr("Reboot ground unit for changes to take effect.")
                    color:          qgcPal.colorOrange
                    visible:        QGroundControl.microhardManager.needReboot
                    font.family:    ScreenTools.demiboldFontFamily
                    anchors.horizontalCenter:   parent.horizontalCenter
                }
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
                                enabled:    !QGroundControl.microhardManager.needReboot
                                visible:    _microhardEnabledFact.visible
                            }
                            FactCheckBox {
                                text:       qsTr("Enable Microhard Video")
                                fact:       _microhardVideoEnabledFact
                                visible:    _microhardVideoEnabledFact.visible
                                enabled:    _microhardEnabled && !QGroundControl.microhardManager.needReboot
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
                                text:           QGroundControl.microhardManager.connected ? qsTr("Connected") : qsTr("Not Connected")
                                color:          QGroundControl.microhardManager.connected ? qgcPal.colorGreen : qgcPal.colorRed
                                Layout.minimumWidth: _valueWidth
                            }
                            QGCLabel {
                                text:           qsTr("Air Unit:")
                            }
                            QGCLabel {
                                text:           QGroundControl.microhardManager.linkConnected ? qsTr("Connected") : qsTr("Not Connected")
                                color:          QGroundControl.microhardManager.linkConnected ? qgcPal.colorGreen : qgcPal.colorRed
                            }
                            QGCLabel {
                                text:           qsTr("Uplink RSSI:")
                            }
                            QGCLabel {
                                text:           QGroundControl.microhardManager.linkConnected ? QGroundControl.microhardManager.uplinkRSSI : ""
                            }
                            QGCLabel {
                                text:           qsTr("Downlink RSSI:")
                            }
                            QGCLabel {
                                text:           QGroundControl.microhardManager.linkConnected ? QGroundControl.microhardManager.downlinkRSSI : ""
                            }
                        }
                    }
                }
                //-----------------------------------------------------------------
                //-- Device Info
                Item {
                    width:                      _panelWidth
                    height:                     devInfoLabel.height
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    visible:                    _microhardEnabled && QGroundControl.microhardManager.connected
                    QGCLabel {
                        id:                     devInfoLabel
                        text:                   qsTr("Device Info")
                        font.family:            ScreenTools.demiboldFontFamily
                    }
                }
                Rectangle {
                    height:                     devInfoCol.height + (ScreenTools.defaultFontPixelHeight * 2)
                    width:                      _panelWidth
                    color:                      qgcPal.windowShade
                    visible:                    _microhardEnabled && QGroundControl.microhardManager.connected
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    Column {
                        id:                     devInfoCol
                        spacing:                ScreenTools.defaultFontPixelHeight * 0.5
                        width:                  parent.width
                        anchors.centerIn:       parent
                        GridLayout {
                            anchors.margins:    ScreenTools.defaultFontPixelHeight
                            columnSpacing:      ScreenTools.defaultFontPixelWidth * 2
                            anchors.horizontalCenter: parent.horizontalCenter
                            columns: 2
                            QGCLabel {
                                text:           qsTr("Serial Number:")
                                Layout.minimumWidth: _labelWidth
                            }
                            QGCLabel {
                                text:           QGroundControl.microhardManager.connected ? QGroundControl.microhardManager.serialNumber : qsTr("")
                                Layout.minimumWidth: _valueWidth
                            }
                            QGCLabel {
                                text:           qsTr("Firmware Version:")
                            }
                            QGCLabel {
                                text:           QGroundControl.microhardManager.connected ? QGroundControl.microhardManager.fwVersion : qsTr("")
                            }
                        }
                    }
                }
                //-----------------------------------------------------------------
                //-- Radio Settings
                Item {
                    width:                      _panelWidth
                    height:                     radioSettingsLabel.height
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    visible:                    _microhardEnabled && QGroundControl.microhardManager.linkConnected
                    QGCLabel {
                        id:                     radioSettingsLabel
                        text:                   qsTr("Radio Settings")
                        font.family:            ScreenTools.demiboldFontFamily
                    }
                }
                Rectangle {
                    height:                     radioSettingsCol.height + (ScreenTools.defaultFontPixelHeight * 2)
                    width:                      _panelWidth
                    color:                      qgcPal.windowShade
                    visible:                    _microhardEnabled && QGroundControl.microhardManager.linkConnected
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    Column {
                        id:                     radioSettingsCol
                        spacing:                ScreenTools.defaultFontPixelHeight * 0.5
                        width:                  parent.width
                        anchors.centerIn:       parent
                        GridLayout {
                            anchors.margins:    ScreenTools.defaultFontPixelHeight
                            columnSpacing:      ScreenTools.defaultFontPixelWidth * 2
                            anchors.horizontalCenter: parent.horizontalCenter
                            columns: 2
                            QGCLabel {
                                text:           qsTr("Radio Mode:")
                                Layout.minimumWidth: _labelWidth
                            }
                            FactComboBox {
                                fact:           QGroundControl.microhardManager.radioMode
                                indexModel:     true
                                enabled:        QGroundControl.microhardManager.linkConnected && !QGroundControl.microhardManager.needReboot
                                Layout.minimumWidth: _valueWidth
                            }
                            QGCLabel {
                                text:           qsTr("Radio Frequency:")
                            }
                            FactComboBox {
                                fact:           QGroundControl.microhardManager.radioChannel
                                indexModel:     true
                                enabled:        QGroundControl.microhardManager.linkConnected && QGroundControl.microhardManager.radioMode.rawValue > 0 && !QGroundControl.microhardManager.needReboot
                                Layout.minimumWidth: _valueWidth
                            }
                        }
                    }
                }
                //-----------------------------------------------------------------
                //-- Video Settings
                Item {
                    width:                      _panelWidth
                    height:                     videoSettingsLabel.height
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    visible:                    _microhardEnabled && QGroundControl.microhardManager.linkConnected
                    QGCLabel {
                        id:                     videoSettingsLabel
                        text:                   qsTr("Video Settings")
                        font.family:            ScreenTools.demiboldFontFamily
                    }
                }
                Rectangle {
                    height:                     videoSettingsCol.height + (ScreenTools.defaultFontPixelHeight * 2)
                    width:                      _panelWidth
                    color:                      qgcPal.windowShade
                    visible:                    _microhardEnabled && QGroundControl.microhardManager.linkConnected
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    Column {
                        id:                     videoSettingsCol
                        spacing:                ScreenTools.defaultFontPixelHeight * 0.5
                        width:                  parent.width
                        anchors.centerIn:       parent
                        GridLayout {
                            anchors.margins:    ScreenTools.defaultFontPixelHeight
                            columnSpacing:      ScreenTools.defaultFontPixelWidth * 2
                            anchors.horizontalCenter: parent.horizontalCenter
                            columns: 2
                            QGCLabel {
                                text:           qsTr("Video Output:")
                                Layout.minimumWidth: _labelWidth
                            }
                            FactComboBox {
                                fact:           QGroundControl.microhardManager.videoOutput
                                indexModel:     true
                                enabled:        QGroundControl.microhardManager.linkConnected && !QGroundControl.microhardManager.needReboot
                                Layout.minimumWidth: _valueWidth
                            }
                            QGCLabel {
                                text:           qsTr("Encoder:")
                            }
                            FactComboBox {
                                fact:           QGroundControl.microhardManager.videoMode
                                indexModel:     true
                                enabled:        QGroundControl.microhardManager.linkConnected && !QGroundControl.microhardManager.needReboot
                                Layout.minimumWidth: _valueWidth
                            }
                            QGCLabel {
                                text:           qsTr("Bit Rate:")
                            }
                            FactComboBox {
                                fact:           QGroundControl.microhardManager.videoRate
                                indexModel:     true
                                enabled:        QGroundControl.microhardManager.linkConnected && !QGroundControl.microhardManager.needReboot
                                Layout.minimumWidth: _valueWidth
                            }
                        }
                    }
                }
                //-----------------------------------------------------------------
                //-- RTSP Settings
                Item {
                    width:                      _panelWidth
                    height:                     rtspSettingsLabel.height
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    visible:                    _microhardEnabled && QGroundControl.microhardManager.connected
                    QGCLabel {
                        id:                     rtspSettingsLabel
                        text:                   qsTr("Streaming Settings")
                        font.family:            ScreenTools.demiboldFontFamily
                    }
                }
                Rectangle {
                    height:                     rtspSettingsCol.height + (ScreenTools.defaultFontPixelHeight * 2)
                    width:                      _panelWidth
                    color:                      qgcPal.windowShade
                    visible:                    _microhardEnabled && QGroundControl.microhardManager.connected
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    Column {
                        id:                     rtspSettingsCol
                        spacing:                ScreenTools.defaultFontPixelHeight * 0.5
                        width:                  parent.width
                        anchors.centerIn:       parent
                        GridLayout {
                            anchors.margins:    ScreenTools.defaultFontPixelHeight
                            columnSpacing:      ScreenTools.defaultFontPixelWidth * 2
                            anchors.horizontalCenter: parent.horizontalCenter
                            columns: 2
                            QGCLabel {
                                text:           qsTr("RTSP URI:")
                                Layout.minimumWidth: _labelWidth
                            }
                            QGCTextField {
                                id:             rtspURI
                                text:           QGroundControl.microhardManager.rtspURI
                                enabled:        QGroundControl.microhardManager.connected && !QGroundControl.microhardManager.needReboot
                                inputMethodHints:    Qt.ImhUrlCharactersOnly
                                Layout.minimumWidth: _valueWidth
                            }
                            QGCLabel {
                                text:           qsTr("Account:")
                            }
                            QGCTextField {
                                id:             rtspAccount
                                text:           QGroundControl.microhardManager.rtspAccount
                                enabled:        QGroundControl.microhardManager.connected && !QGroundControl.microhardManager.needReboot
                                Layout.minimumWidth: _valueWidth
                            }
                            QGCLabel {
                                text:           qsTr("Password:")
                            }
                            QGCTextField {
                                id:             rtspPassword
                                text:           QGroundControl.microhardManager.rtspPassword
                                enabled:        QGroundControl.microhardManager.connected && !QGroundControl.microhardManager.needReboot
                                inputMethodHints:    Qt.ImhHiddenText
                                Layout.minimumWidth: _valueWidth
                            }
                        }
                        Item {
                            width:  1
                            height: ScreenTools.defaultFontPixelHeight
                        }
                        QGCButton {
                            function testEnabled() {
                                if(!QGroundControl.microhardManager.connected)
                                    return false
                                if(rtspPassword.text === QGroundControl.microhardManager.rtspPassword &&
                                    rtspAccount.text === QGroundControl.microhardManager.rtspAccount &&
                                    rtspURI.text     === QGroundControl.microhardManager.rtspURI)
                                    return false
                                if(rtspURI === "")
                                    return false
                                return true
                            }
                            enabled:            testEnabled() && !QGroundControl.microhardManager.needReboot
                            text:               qsTr("Apply")
                            anchors.horizontalCenter:   parent.horizontalCenter
                            onClicked: {
                                setRTSPDialog.open()
                            }
                            MessageDialog {
                                id:                 setRTSPDialog
                                icon:               StandardIcon.Warning
                                standardButtons:    StandardButton.Yes | StandardButton.No
                                title:              qsTr("Set Streaming Settings")
                                text:               qsTr("Once changed, you will need to reboot the ground unit for the changes to take effect.\n\nConfirm change?")
                                onYes: {
                                    QGroundControl.microhardManager.setRTSPSettings(rtspURI.text, rtspAccount.text, rtspPassword.text)
                                    setRTSPDialog.close()
                                }
                                onNo: {
                                    setRTSPDialog.close()
                                }
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
                    visible:                    _microhardEnabled && (!ScreenTools.isiOS && !ScreenTools.isAndroid)
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
                    visible:                    _microhardEnabled && (!ScreenTools.isiOS && !ScreenTools.isAndroid)
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
                                enabled:        !QGroundControl.microhardManager.needReboot
                                inputMethodHints:    Qt.ImhFormattedNumbersOnly
                                Layout.minimumWidth: _valueWidth
                            }
                            QGCLabel {
                                text:           qsTr("Ground Unit IP Address:")
                            }
                            QGCTextField {
                                id:             remoteIP
                                text:           QGroundControl.microhardManager.remoteIPAddr
                                enabled:        !QGroundControl.microhardManager.needReboot
                                inputMethodHints:    Qt.ImhFormattedNumbersOnly
                                Layout.minimumWidth: _valueWidth
                            }
                            QGCLabel {
                                text:           qsTr("Network Mask:")
                            }
                            QGCTextField {
                                id:             netMask
                                text:           QGroundControl.microhardManager.netMask
                                enabled:        !QGroundControl.microhardManager.needReboot
                                inputMethodHints:    Qt.ImhFormattedNumbersOnly
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
                                if(localIP.text   === QGroundControl.microhardManager.localIPAddr &&
                                    remoteIP.text === QGroundControl.microhardManager.remoteIPAddr &&
                                    netMask.text  === QGroundControl.microhardManager.netMask)
                                    return false
                                if(!validateIPaddress(localIP.text))  return false
                                if(!validateIPaddress(remoteIP.text)) return false
                                if(!validateIPaddress(netMask.text))  return false
                                return true
                            }
                            enabled:            testEnabled() && !QGroundControl.microhardManager.needReboot
                            text:               qsTr("Apply")
                            anchors.horizontalCenter:   parent.horizontalCenter
                            onClicked: {
                                setIPDialog.open()
                            }
                            MessageDialog {
                                id:                 setIPDialog
                                icon:               StandardIcon.Warning
                                standardButtons:    StandardButton.Yes | StandardButton.No
                                title:              qsTr("Set Network Settings")
                                text:               qsTr("Once changed, you will need to reboot the ground unit for the changes to take effect. The local IP address must match the one entered (%1).\n\nConfirm change?").arg(localIP.text)
                                onYes: {
                                    QGroundControl.microhardManager.setIPSettings(localIP.text, remoteIP.text, netMask.text)
                                    setIPDialog.close()
                                }
                                onNo: {
                                    setIPDialog.close()
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

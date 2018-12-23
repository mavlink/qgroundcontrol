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
    property Fact _taisyncEnabledFact:          QGroundControl.settingsManager.appSettings.enableTaisync
    property Fact _taisyncVideoEnabledFact:     QGroundControl.settingsManager.appSettings.enableTaisyncVideo
    property bool _taisyncEnabled:              _taisyncEnabledFact.rawValue

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
                                text:       qsTr("Enable Taisync")
                                fact:       _taisyncEnabledFact
                                visible:    _taisyncEnabledFact.visible
                            }
                            FactCheckBox {
                                text:       qsTr("Enable Taisync Video")
                                fact:       _taisyncVideoEnabledFact
                                visible:    _taisyncVideoEnabledFact.visible
                                enabled:    _taisyncEnabled
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
                    visible:                    _taisyncEnabled
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
                    visible:                    _taisyncEnabled
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
                                text:           QGroundControl.taisyncManager.connected ? qsTr("Connected") : qsTr("Not Connected")
                                color:          QGroundControl.taisyncManager.connected ? qgcPal.colorGreen : qgcPal.colorRed
                                Layout.minimumWidth: _valueWidth
                            }
                            QGCLabel {
                                text:           qsTr("Air Unit:")
                            }
                            QGCLabel {
                                text:           QGroundControl.taisyncManager.linkConnected ? qsTr("Connected") : qsTr("Not Connected")
                                color:          QGroundControl.taisyncManager.linkConnected ? qgcPal.colorGreen : qgcPal.colorRed
                            }
                            QGCLabel {
                                text:           qsTr("Uplink RSSI:")
                            }
                            QGCLabel {
                                text:           QGroundControl.taisyncManager.linkConnected ? QGroundControl.taisyncManager.uplinkRSSI : ""
                            }
                            QGCLabel {
                                text:           qsTr("Downlink RSSI:")
                            }
                            QGCLabel {
                                text:           QGroundControl.taisyncManager.linkConnected ? QGroundControl.taisyncManager.downlinkRSSI : ""
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
                    visible:                    _taisyncEnabled && QGroundControl.taisyncManager.connected
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
                    visible:                    _taisyncEnabled && QGroundControl.taisyncManager.connected
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
                                text:           QGroundControl.taisyncManager.linkConnected ? QGroundControl.taisyncManager.serialNumber : qsTr("")
                                Layout.minimumWidth: _valueWidth
                            }
                            QGCLabel {
                                text:           qsTr("Firmware Version:")
                            }
                            QGCLabel {
                                text:           QGroundControl.taisyncManager.linkConnected ? QGroundControl.taisyncManager.fwVersion : qsTr("")
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
                    visible:                    _taisyncEnabled && QGroundControl.taisyncManager.linkConnected
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
                    visible:                    _taisyncEnabled && QGroundControl.taisyncManager.linkConnected
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
                                fact:           QGroundControl.taisyncManager.radioMode
                                indexModel:     true
                                enabled:        QGroundControl.taisyncManager.linkConnected
                                Layout.minimumWidth: _valueWidth
                            }
                            QGCLabel {
                                text:           qsTr("Radio Frequency:")
                            }
                            FactComboBox {
                                fact:           QGroundControl.taisyncManager.radioChannel
                                indexModel:     true
                                enabled:        QGroundControl.taisyncManager.linkConnected && QGroundControl.taisyncManager.radioMode.rawValue > 0
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
                    visible:                    _taisyncEnabled && QGroundControl.taisyncManager.linkConnected
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
                    visible:                    _taisyncEnabled && QGroundControl.taisyncManager.linkConnected
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
                                fact:           QGroundControl.taisyncManager.videoOutput
                                indexModel:     true
                                enabled:        QGroundControl.taisyncManager.linkConnected
                                Layout.minimumWidth: _valueWidth
                            }
                            QGCLabel {
                                text:           qsTr("Encoder:")
                            }
                            FactComboBox {
                                fact:           QGroundControl.taisyncManager.videoMode
                                indexModel:     true
                                enabled:        QGroundControl.taisyncManager.linkConnected
                                Layout.minimumWidth: _valueWidth
                            }
                            QGCLabel {
                                text:           qsTr("Bit Rate:")
                            }
                            FactComboBox {
                                fact:           QGroundControl.taisyncManager.videoRate
                                indexModel:     true
                                enabled:        QGroundControl.taisyncManager.linkConnected
                                Layout.minimumWidth: _valueWidth
                            }
                        }
                    }
                }
            }
        }
    }
}

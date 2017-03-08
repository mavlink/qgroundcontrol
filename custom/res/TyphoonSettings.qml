/*!
 * @file
 * @brief ST16 Settings Panel
 * @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick                  2.5
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.1

import QGroundControl                       1.0
import QGroundControl.FactSystem            1.0
import QGroundControl.FactControls          1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Palette               1.0
import QGroundControl.Controllers           1.0
import TyphoonHQuickInterface               1.0

QGCView {
    id:                 qgcView
    viewPanel:          panel
    color:              qgcPal.window
    anchors.fill:       parent
    anchors.margins:    ScreenTools.defaultFontPixelWidth

    property real _labelWidth:                  ScreenTools.defaultFontPixelWidth * 15
    property real _editFieldWidth:              ScreenTools.defaultFontPixelWidth * 30
    property var  _selectedSSID:                ""

    ExclusiveGroup  { id: ssidGroup }
    QGCPalette      { id: qgcPal }

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
                width:              qgcView.width
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                anchors.margins:    ScreenTools.defaultFontPixelWidth
                //-----------------------------------------------------------------
                //-- Bind
                Item {
                    width:              qgcView.width * 0.8
                    height:             rcBindLabel.height
                    anchors.margins:    ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    QGCLabel {
                        id:             rcBindLabel
                        text:           qsTr("RC Binding")
                        font.family:    ScreenTools.demiboldFontFamily
                    }
                }
                Rectangle {
                    height:         rcBindRow.height + (ScreenTools.defaultFontPixelHeight * 2)
                    width:          qgcView.width * 0.8
                    color:          qgcPal.windowShade
                    anchors.margins: ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    Row {
                        id:         rcBindRow
                        spacing:    ScreenTools.defaultFontPixelWidth
                        anchors.verticalCenter: parent.verticalCenter
                        Item {
                            width:              ScreenTools.defaultFontPixelWidth
                            height:             1
                        }
                        QGCButton {
                            text:       "Bind"
                            width:      _labelWidth
                            enabled:    QGroundControl.multiVehicleManager.activeVehicle
                            onClicked:  TyphoonHQuickInterface.enterBindMode()
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Item {
                            width:              ScreenTools.defaultFontPixelWidth * 2
                            height:             1
                        }
                        QGCLabel {
                            width:      _editFieldWidth
                            text:       TyphoonHQuickInterface.m4StateStr
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }
                //-----------------------------------------------------------------
                //-- Bind
                Item {
                    width:              qgcView.width * 0.8
                    height:             wifiBindLabel.height
                    anchors.margins:    ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    QGCLabel {
                        id:             wifiBindLabel
                        text:           qsTr("Telemetry/Video Binding")
                        font.family:    ScreenTools.demiboldFontFamily
                    }
                }
                Rectangle {
                    height:         scanCol.height + (ScreenTools.defaultFontPixelHeight * 2)
                    width:          qgcView.width * 0.8
                    color:          qgcPal.windowShade
                    anchors.margins: ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    Column {
                        id:         scanCol
                        spacing:    ScreenTools.defaultFontPixelHeight * 0.25
                        anchors.horizontalCenter: parent.horizontalCenter
                        Item {
                            width:  1
                            height: ScreenTools.defaultFontPixelHeight
                        }
                        Row {
                            spacing:        ScreenTools.defaultFontPixelWidth
                            QGCButton {
                                text:       "Start Scan"
                                width:      _labelWidth
                                onClicked:  TyphoonHQuickInterface.startScan()
                                enabled:    !TyphoonHQuickInterface.scanningWiFi
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            QGCButton {
                                text:       "Stop Scan"
                                width:      _labelWidth
                                onClicked:  TyphoonHQuickInterface.stopScan()
                                enabled:    TyphoonHQuickInterface.scanningWiFi
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            QGCButton {
                                text:       "Bind"
                                width:      _labelWidth
                                enabled:    _selectedSSID !== ""
                                primary:    _selectedSSID !== ""
                                onClicked:  TyphoonHQuickInterface.bindWIFI(_selectedSSID)
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                        Item {
                            width:      1
                            height:     TyphoonHQuickInterface.ssidList.length > 0 ? ScreenTools.defaultFontPixelHeight : 0
                        }
                        Repeater {
                            model:          TyphoonHQuickInterface.ssidList
                            delegate:
                            QGCButton {
                                width:      ScreenTools.defaultFontPixelWidth * 36
                                text:       modelData
                                exclusiveGroup: ssidGroup
                                onClicked:  {
                                    if(_selectedSSID === modelData) {
                                        _selectedSSID = ""
                                        checked = false
                                    } else {
                                        _selectedSSID = modelData
                                        checked = true
                                    }
                                }
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                        }
                    }
                }
                QGCLabel {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("QGroundControl Version: " + QGroundControl.qgcVersion)
                }
            }
        }
    }
}

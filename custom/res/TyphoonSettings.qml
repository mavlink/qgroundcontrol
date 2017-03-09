/*!
 * @file
 * @brief ST16 Settings Panel
 * @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick                  2.5
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.2

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
    property var  _telemetryText:               qsTr("Telemetry/Video Connection")

    ExclusiveGroup  { id: ssidGroup }
    QGCPalette      { id: qgcPal }

    Connections {
        target: TyphoonHQuickInterface
        onAuthenticationError: {
            authErrorDialog.visible = true
        }
    }

    function getWiFiStatus() {
        if(TyphoonHQuickInterface.bindingWiFi) {
            return qsTr("Connecting...")
        }
        if(TyphoonHQuickInterface.scanningWiFi) {
            return qsTr("Scanning...")
        }
        if(TyphoonHQuickInterface.connectedSSID != "") {
            return qsTr("Connected to ") + TyphoonHQuickInterface.connectedSSID
        }
        return ""
    }

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
                //-- RX/TX Bind
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
                            text:       qsTr("Bind")
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
                //-- WIFI Bind
                Item {
                    width:              qgcView.width * 0.8
                    height:             wifiBindLabel.height
                    anchors.margins:    ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    QGCLabel {
                        id:             wifiBindLabel
                        text:           _telemetryText
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
                            anchors.horizontalCenter: parent.horizontalCenter
                            QGCButton {
                                text:       qsTr("Refresh")
                                width:      _labelWidth
                                onClicked:  TyphoonHQuickInterface.startScan()
                                enabled:    !TyphoonHQuickInterface.scanningWiFi
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            QGCButton {
                                text:       qsTr("Bind")
                                width:      _labelWidth
                                enabled:    _selectedSSID !== "" && _selectedSSID != TyphoonHQuickInterface.connectedSSID
                                primary:    _selectedSSID !== "" && _selectedSSID != TyphoonHQuickInterface.connectedSSID
                                onClicked: {
                                    passwordDialog.visible = true
                                }
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                        Item {
                            width:      1
                            height:     TyphoonHQuickInterface.ssidList.length > 0 ? ScreenTools.defaultFontPixelHeight : 0
                        }
                        Item {
                            width:      wifiStatusLabel.width
                            height:     wifiStatusLabel.height * 2
                            visible:    wifiStatusLabel.text !== ""
                            anchors.horizontalCenter: parent.horizontalCenter
                            QGCLabel {
                                id:             wifiStatusLabel
                                text:           getWiFiStatus()
                                anchors.centerIn: parent
                            }
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

    Rectangle {
        id:         authErrorDialog
        width:      badPpwdCol.width  * 1.5
        height:     badPpwdCol.height * 1.5
        radius:     ScreenTools.defaultFontPixelWidth * 0.5
        color:      qgcPal.window
        visible:    false
        border.color: qgcPal.text
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        Keys.onBackPressed: {
            authErrorDialog.visible = false
        }
        Column {
            id:         badPpwdCol
            spacing:    ScreenTools.defaultFontPixelHeight
            anchors.centerIn: parent
            QGCLabel {
                text:   qsTr("Invalid ") + _telemetryText + qsTr(" Password")
                anchors.horizontalCenter: parent.horizontalCenter
            }
            QGCButton {
                text:       qsTr("Close")
                width:      _labelWidth
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked:  {
                    authErrorDialog.visible = false
                }
            }
        }
    }

    Rectangle {
        id:         passwordDialog
        width:      pwdCol.width  * 1.25
        height:     pwdCol.height * 1.25
        radius:     ScreenTools.defaultFontPixelWidth * 0.5
        color:      qgcPal.window
        visible:    false
        border.color: qgcPal.text
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        Keys.onBackPressed: {
            passwordDialog.visible = false
        }
        Column {
            id:         pwdCol
            spacing:    ScreenTools.defaultFontPixelHeight
            anchors.centerIn: parent
            QGCLabel {
                text:   qsTr("Please enter password for ") + _selectedSSID
                anchors.horizontalCenter: parent.horizontalCenter
            }
            QGCTextField {
                id:         passwordField
                echoMode:   TextInput.Password
                width:      ScreenTools.defaultFontPixelWidth * 20
                focus:      true
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Row {
                spacing:    ScreenTools.defaultFontPixelWidth * 4
                anchors.horizontalCenter: parent.horizontalCenter
                QGCButton {
                    text:       qsTr("Ok")
                    width:      _labelWidth
                    enabled:    passwordField.text.length > 7
                    onClicked:  {
                        Qt.inputMethod.hide();
                        TyphoonHQuickInterface.bindWIFI(_selectedSSID, passwordField.text)
                        _selectedSSID = ""
                        passwordDialog.visible = false
                    }
                }
                QGCButton {
                    text:       qsTr("Cancel")
                    width:      _labelWidth
                    onClicked:  {
                        Qt.inputMethod.hide();
                        passwordDialog.visible = false
                    }
                }
            }
        }
    }

}

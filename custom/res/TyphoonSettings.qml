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
import TyphoonHQuickInterface.Widgets       1.0

QGCView {
    id:                 qgcView
    viewPanel:          panel
    color:              qgcPal.window
    anchors.fill:       parent
    anchors.margins:    ScreenTools.defaultFontPixelWidth

    property real _labelWidth:                  ScreenTools.defaultFontPixelWidth * 15
    property real _editFieldWidth:              ScreenTools.defaultFontPixelWidth * 30
    property var  _selectedSSID:                ""
    property var  _connectText:                 qsTr("Connect")

    ExclusiveGroup  { id: ssidGroup }
    QGCPalette      { id: qgcPal }

    Connections {
        target: TyphoonHQuickInterface
        onAuthenticationError: {
            QGroundControl.skipSetupPage = false
            authErrorDialog.visible = true
        }
        onWifiConnected: {
            QGroundControl.skipSetupPage = false
            TyphoonHQuickInterface.stopScan();
            mainWindow.showFlyView()
        }
        onScanningWiFiChanged: {
            if(TyphoonHQuickInterface.scanningWiFi) {
                imageRotation.start()
            }
        }
    }

    Component.onDestruction: {
        TyphoonHQuickInterface.stopScan();
    }

    Component.onCompleted: {
        TyphoonHQuickInterface.startScan();
    }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent
        QGCLabel {
            text:           qsTr("Connecting...")
            visible:        TyphoonHQuickInterface.bindingWiFi
            font.family:    ScreenTools.demiboldFontFamily
            anchors.centerIn: parent
        }
        QGCFlickable {
            clip:               true
            anchors.fill:       parent
            contentHeight:      settingsColumn.height
            contentWidth:       settingsColumn.width
            visible:            !TyphoonHQuickInterface.bindingWiFi
            Column {
                id:                 settingsColumn
                width:              qgcView.width
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                anchors.margins:    ScreenTools.defaultFontPixelWidth
                //-----------------------------------------------------------------
                //-- WIFI AP Connection
                Item {
                    width:              qgcView.width * 0.8
                    height:             resetButton.height
                    anchors.margins:    ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    QGCLabel {
                        id:             wifiBindLabel
                        anchors.left:   parent.left
                        text: {
                            if(TyphoonHQuickInterface.connectedSSID != "") {
                                return qsTr("Connected to ") + TyphoonHQuickInterface.connectedSSID
                            } else {
                                return qsTr("Connected to Vehicle")
                            }
                        }
                        font.family:    ScreenTools.demiboldFontFamily
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    QGCButton {
                        id:             resetButton
                        text:           qsTr("Reset")
                        anchors.right:  parent.right
                        onClicked: {
                            QGroundControl.skipSetupPage = true
                            if(QGroundControl.multiVehicleManager.activeVehicle) {
                                QGroundControl.multiVehicleManager.activeVehicle.autoDisconnect = true;
                            }
                            TyphoonHQuickInterface.resetWifi();
                        }
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Rectangle {
                    height:         scanCol.height + (ScreenTools.defaultFontPixelHeight * 2)
                    width:          qgcView.width * 0.8
                    color:          qgcPal.windowShade
                    visible:        !TyphoonHQuickInterface.bindingWiFi
                    anchors.margins: ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    Column {
                        id:         scanCol
                        spacing:    ScreenTools.defaultFontPixelHeight * 0.25
                        width:      ScreenTools.defaultFontPixelWidth * 40
                        anchors.horizontalCenter: parent.horizontalCenter
                        Item {
                            width:  1
                            height: ScreenTools.defaultFontPixelHeight
                        }
                        Item {
                            width:  ScreenTools.defaultFontPixelWidth * 36
                            height: scanningIcon.height
                            anchors.horizontalCenter: parent.horizontalCenter
                            QGCLabel {
                                text:           qsTr("Select Vehicle to Connect")
                                anchors.left:   parent.left
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            QGCColoredImage {
                                id:                 scanningIcon
                                height:             ScreenTools.defaultFontPixelHeight * 2
                                width:              height
                                source:             "/qmlimages/MapSync.svg"
                                sourceSize.height:  height
                                fillMode:           Image.PreserveAspectFit
                                mipmap:             true
                                smooth:             true
                                color:              qgcPal.buttonText
                                anchors.right:      parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                RotationAnimation on rotation {
                                    id:             imageRotation
                                    loops:          1
                                    from:           0
                                    to:             360
                                    duration:       500
                                    running:        false
                                }
                                MouseArea {
                                    anchors.fill:   parent
                                    onClicked:      TyphoonHQuickInterface.startScan();
                                }
                            }
                        }
                        Repeater {
                            model:          TyphoonHQuickInterface.ssidList
                            delegate:
                            SSIDButton {
                                exclusiveGroup:     ssidGroup
                                text:               modelData
                                source:             "qrc:/typhoonh/checkMark.svg"
                                showIcon:           TyphoonHQuickInterface.connectedSSID === modelData
                                enabled:            TyphoonHQuickInterface.connectedSSID !== modelData
                                anchors.horizontalCenter:   parent.horizontalCenter
                                onClicked:  {
                                    if(_selectedSSID === modelData) {
                                        _selectedSSID = ""
                                        checked = false
                                    } else {
                                        _selectedSSID = modelData
                                        checked = true
                                        passwordDialog.visible = true
                                    }
                                }
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
    //-- Authentication Error Dialog
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
                text:   qsTr("Invalid ") + _connectText + qsTr(" Password")
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
    //-- Password Dialog
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
                        QGroundControl.skipSetupPage = true
                        //-- If we were connected to something, let it go away when it disconnects.
                        if(QGroundControl.multiVehicleManager.activeVehicle) {
                            QGroundControl.multiVehicleManager.activeVehicle.autoDisconnect = true;
                        }
                        TyphoonHQuickInterface.bindWIFI(_selectedSSID, passwordField.text)
                        _selectedSSID = ""
                        passwordField.text = ""
                        passwordDialog.visible = false
                    }
                }
                QGCButton {
                    text:       qsTr("Cancel")
                    width:      _labelWidth
                    onClicked:  {
                        Qt.inputMethod.hide();
                        ssidGroup.current = null
                        passwordField.text = ""
                        passwordDialog.visible = false
                    }
                }
            }
        }
    }
}

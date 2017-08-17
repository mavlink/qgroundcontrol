/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick              2.3
import QtQuick.Controls     1.2
import QtQuick.Layouts      1.2
import QtGraphicalEffects   1.0

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

import TyphoonHQuickInterface               1.0
import TyphoonHQuickInterface.Widgets       1.0

//-------------------------------------------------------------------------
//-- WIFI RSSI Indicator
Item {
    width:          ScreenTools.defaultFontPixelWidth * 8
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property bool _hasWifi:     TyphoonHQuickInterface.rssi > -100 && TyphoonHQuickInterface.rssi < 0
    property bool _isAP:        _hasWifi && !TyphoonHQuickInterface.isTyphoon

    function hideWiFiManagement() {
        rootLoader.sourceComponent = null
        mainWindow.enableToolbar()
    }

    Component {
        id: wifiRSSIInfo
        Rectangle {
            id:     wifiInfoRect
            width:  rcrssiCol.width   + ScreenTools.defaultFontPixelWidth  * 3
            height: rcrssiCol.height  + ScreenTools.defaultFontPixelHeight * 2
            radius: ScreenTools.defaultFontPixelHeight * 0.5
            color:  qgcPal.window
            border.color:   qgcPal.text

            Column {
                id:                 rcrssiCol
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                width:              Math.max(rcrssiGrid.width, rssiLabel.width)
                anchors.margins:    ScreenTools.defaultFontPixelHeight
                anchors.centerIn:   parent

                QGCLabel {
                    id:             rssiLabel
                    text:           _hasWifi ? qsTr("Video/Telemetry RSSI Status") : qsTr("Video/Telemetry Link Data Unavailable")
                    font.family:    ScreenTools.demiboldFontFamily
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                GridLayout {
                    id:                 rcrssiGrid
                    visible:            _hasWifi
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    columnSpacing:      ScreenTools.defaultFontPixelWidth
                    columns:            2
                    anchors.horizontalCenter: parent.horizontalCenter

                    QGCLabel { text: qsTr("Connected to:"); visible: TyphoonHQuickInterface.connectedSSID !== ""; }
                    QGCLabel { text: TyphoonHQuickInterface.connectedSSID; visible: TyphoonHQuickInterface.connectedSSID !== ""; }
                    QGCLabel { text: qsTr("RSSI:") }
                    QGCLabel { text: TyphoonHQuickInterface.rssi + "dB" }
                }

                Item {
                    width:  1
                    height: ScreenTools.defaultFontPixelHeight
                }

                QGCButton {
                    text:   "Link Management"
                    onClicked: {
                        rootLoader.sourceComponent = wifiManagement
                        mainWindow.disableToolbar()
                        if(mainWindow.currentPopUp) {
                            mainWindow.currentPopUp.close()
                        }
                    }
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }

            Component.onCompleted: {
                var pos = mapFromItem(toolBar, centerX - (width / 2), toolBar.height)
                x = pos.x
                y = pos.y + ScreenTools.defaultFontPixelHeight
                if((x + width) >= toolBar.width) {
                    x = toolBar.width - width - ScreenTools.defaultFontPixelWidth;
                }
            }
        }
    }

    //-------------------------------------------------------------------------
    //-- Wi-Fi Link Management
    Component {
        id:             wifiManagement
        Item {
            id:         wifiManagementItem
            z:          1000000
            width:      mainWindow.width
            height:     mainWindow.height

            property var  _activeVehicle:       QGroundControl.multiVehicleManager.activeVehicle
            property real _labelWidth:          ScreenTools.defaultFontPixelWidth * 15
            property real _editFieldWidth:      ScreenTools.defaultFontPixelWidth * 30
            property var  _selectedSSID:        ""
            property var  _connectText:         qsTr("Connect")

            ExclusiveGroup  { id: ssidGroup }

            function connectWifi(password) {
                QGroundControl.skipSetupPage = true
                //-- If we were connected to something, let it go away when it disconnects.
                if(QGroundControl.multiVehicleManager.activeVehicle) {
                    QGroundControl.multiVehicleManager.activeVehicle.autoDisconnect = true;
                }
                TyphoonHQuickInterface.bindWIFI(_selectedSSID, password)
                _selectedSSID = ""
                passwordLoader.sourceComponent = null
            }

            Connections {
                target: TyphoonHQuickInterface
                onAuthenticationError: {
                    QGroundControl.skipSetupPage = false
                    authErrorDialog.visible = true
                }
                onWifiConnectedChanged: {
                    if(TyphoonHQuickInterface.connected) {
                        QGroundControl.skipSetupPage = false
                        TyphoonHQuickInterface.stopScan();
                        mainWindow.showFlyView()
                        hideWiFiManagement()
                    }
                }
                onScanningWiFiChanged: {
                    if(TyphoonHQuickInterface.scanningWiFi) {
                        imageRotation.start()
                    }
                }
                onBindTimeout: {
                    QGroundControl.skipSetupPage = false
                    timeoutDialog.visible = true;
                }
            }

            Component.onDestruction: {
                TyphoonHQuickInterface.stopScan();
            }

            Component.onCompleted: {
                TyphoonHQuickInterface.startScan();
                rootLoader.width  = wifiManagementItem.width
                rootLoader.height = wifiManagementItem.height
                Qt.inputMethod.hide();
            }

            MouseArea {
                anchors.fill:   parent
                onWheel:        { wheel.accepted = true; }
                onPressed:      { mouse.accepted = true; }
                onReleased:     { mouse.accepted = true; }
            }

            Rectangle {
                id:             connectingShadow
                anchors.fill:   connectingRect
                radius:         connectingRect.radius
                color:          qgcPal.window
                visible:        false
            }
            DropShadow {
                anchors.fill:       connectingShadow
                visible:            connectingRect.visible
                horizontalOffset:   4
                verticalOffset:     4
                radius:             32.0
                samples:            65
                color:              Qt.rgba(0,0,0,0.75)
                source:             connectingShadow
            }
            Rectangle {
                id:     connectingRect
                width:  mainWindow.width  * 0.45
                height: mainWindow.height * 0.45
                radius: ScreenTools.defaultFontPixelWidth
                color:  qgcPal.window
                visible:            TyphoonHQuickInterface.bindingWiFi
                border.width:       1
                border.color:       qgcPal.text
                anchors.centerIn:   parent
                QGCLabel {
                    text:               qsTr("Connecting...")
                    font.family:        ScreenTools.demiboldFontFamily
                    anchors.top:        parent.top
                    anchors.topMargin:  parent.height * 0.3333
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                QGCColoredImage {
                    height:             ScreenTools.defaultFontPixelHeight * 2
                    width:              height
                    source:             "/qmlimages/MapSync.svg"
                    sourceSize.height:  height
                    fillMode:           Image.PreserveAspectFit
                    mipmap:             true
                    smooth:             true
                    color:              qgcPal.buttonText
                    visible:            TyphoonHQuickInterface.bindingWiFi
                    anchors.centerIn:   parent
                    RotationAnimation on rotation {
                        id:             connectionRotation
                        loops:          Animation.Infinite
                        from:           360
                        to:             0
                        duration:       740
                        running:        TyphoonHQuickInterface.bindingWiFi
                    }
                }
            }
            Rectangle {
                id:             wifiShadow
                anchors.fill:   wifiRect
                radius:         wifiRect.radius
                color:          qgcPal.window
                visible:        false
            }
            DropShadow {
                anchors.fill:       wifiShadow
                visible:            wifiRect.visible
                horizontalOffset:   4
                verticalOffset:     4
                radius:             32.0
                samples:            65
                color:              Qt.rgba(0,0,0,0.75)
                source:             wifiShadow
            }
            Rectangle {
                id:     wifiRect
                width:  mainWindow.width  * 0.5
                height: mainWindow.height * 0.75
                radius: ScreenTools.defaultFontPixelWidth
                color:  qgcPal.window
                border.width:       1
                border.color:       qgcPal.text
                visible:            !TyphoonHQuickInterface.bindingWiFi
                anchors.centerIn:   parent
                Column {
                    spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                    anchors.top:        parent.top
                    anchors.topMargin:  ScreenTools.defaultFontPixelHeight
                    anchors.horizontalCenter: parent.horizontalCenter
                    Row {
                        spacing:    ScreenTools.defaultFontPixelWidth
                        height:     scanningIcon.height
                        anchors.horizontalCenter: parent.horizontalCenter
                        QGCLabel {
                            text:           qsTr("Select Vehicle to Connect")
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
                            anchors.verticalCenter: parent.verticalCenter
                            RotationAnimation on rotation {
                                id:             imageRotation
                                loops:          1
                                from:           360
                                to:             0
                                duration:       750
                                running:        false
                            }
                            MouseArea {
                                anchors.fill:   parent
                                onClicked:      TyphoonHQuickInterface.startScan();
                            }
                        }
                    }
                    QGCLabel {
                        text:  qsTr("It may take up to 60 seconds for the vehicle WiFi to appear.")
                        font.pointSize: ScreenTools.smallFontPointSize
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
                QGCFlickable {
                    clip:               true
                    width:              scanCol.width
                    height:             Math.min(wifiRect.height * 0.666, scanCol.height)
                    contentHeight:      scanCol.height
                    contentWidth:       scanCol.width
                    visible:            !TyphoonHQuickInterface.bindingWiFi
                    anchors.centerIn:   parent
                    Column {
                        id:         scanCol
                        spacing:    ScreenTools.defaultFontPixelHeight * 0.25
                        width:      ScreenTools.defaultFontPixelWidth * 40
                        anchors.centerIn: parent
                        Repeater {
                            model:          TyphoonHQuickInterface.ssidList
                            delegate:
                            SSIDButton {
                                exclusiveGroup:     ssidGroup
                                text:               modelData.ssid
                                rssi:               modelData.rssi
                                source:             "qrc:/typhoonh/img/checkMark.svg"
                                showIcon:           TyphoonHQuickInterface.connectedSSID === modelData.ssid
                                enabled:            _activeVehicle ? !_activeVehicle.armed && TyphoonHQuickInterface.connectedSSID !== modelData.ssid : TyphoonHQuickInterface.connectedSSID !== modelData.ssid
                                anchors.horizontalCenter:   parent.horizontalCenter
                                onClicked:  {
                                    if(_selectedSSID === modelData.ssid) {
                                        _selectedSSID = ""
                                        checked = false
                                    } else {
                                        _selectedSSID = modelData.ssid
                                        checked = true
                                        //-- If we have the config, use it (don't ask for password)
                                        if(TyphoonHQuickInterface.isWifiConfigured(_selectedSSID)) {
                                            connectWifi("")
                                        } else {
                                            passwordLoader.sourceComponent = passwordDialog
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                Row {
                    spacing:                ScreenTools.defaultFontPixelWidth * 4
                    visible:                !TyphoonHQuickInterface.bindingWiFi
                    anchors.bottom:         parent.bottom
                    anchors.bottomMargin:   ScreenTools.defaultFontPixelHeight
                    anchors.horizontalCenter: parent.horizontalCenter
                    QGCButton {
                        text:           qsTr("Reset All Links")
                        width:          ScreenTools.defaultFontPixelWidth  * 16
                        height:         ScreenTools.defaultFontPixelHeight * 2
                        //-- Don't allow restting if vehicle is connected and armed
                        enabled:        _activeVehicle ? !_activeVehicle.armed : true
                        onClicked: {
                            QGroundControl.skipSetupPage = true
                            if(_activeVehicle) {
                                _activeVehicle.autoDisconnect = true;
                            }
                            TyphoonHQuickInterface.resetWifi();
                        }
                    }
                    QGCButton {
                        text:           qsTr("Close")
                        width:          ScreenTools.defaultFontPixelWidth  * 16
                        height:         ScreenTools.defaultFontPixelHeight * 2
                        onClicked: {
                            hideWiFiManagement()
                        }
                    }
                }
                //-- Connection Timeout
                Rectangle {
                    id:         timeoutDialog
                    width:      timeoutCol.width  * 1.5
                    height:     timeoutCol.height * 1.5
                    radius:     ScreenTools.defaultFontPixelWidth * 0.5
                    color:      qgcPal.window
                    visible:    false
                    border.width:   1
                    border.color:   qgcPal.text
                    anchors.top:    parent.top
                    anchors.horizontalCenter: parent.horizontalCenter
                    Keys.onBackPressed: {
                        timeoutDialog.visible = false
                    }
                    MouseArea {
                        anchors.fill:   parent
                        onWheel:        { wheel.accepted = true; }
                        onPressed:      { mouse.accepted = true; }
                        onReleased:     { mouse.accepted = true; }
                    }
                    Column {
                        id:         timeoutCol
                        spacing:    ScreenTools.defaultFontPixelHeight
                        anchors.centerIn: parent
                        QGCLabel {
                            text:   qsTr("Connection Timeout")
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        QGCButton {
                            text:       qsTr("Close")
                            width:      _labelWidth
                            anchors.horizontalCenter: parent.horizontalCenter
                            onClicked:  {
                                timeoutDialog.visible = false
                            }
                        }
                    }
                }
                //-- Authentication Error Dialog
                Rectangle {
                    id:         authErrorDialog
                    width:      badPpwdCol.width  * 3
                    height:     badPpwdCol.height * 3
                    radius:     ScreenTools.defaultFontPixelWidth * 0.5
                    color:      qgcPal.window
                    visible:    false
                    border.width:       1
                    border.color:       qgcPal.text
                    anchors.centerIn:   parent
                    Keys.onBackPressed: {
                        authErrorDialog.visible = false
                    }
                    MouseArea {
                        anchors.fill:   parent
                        onWheel:        { wheel.accepted = true; }
                        onPressed:      { mouse.accepted = true; }
                        onReleased:     { mouse.accepted = true; }
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
                Loader {
                    id:             passwordLoader
                    anchors.top:    parent.top
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                //-- Password Dialog
                Component {
                    id:  passwordDialog
                    Rectangle {
                        id:         pwdRect
                        width:      pwdCol.width  * 1.25
                        height:     pwdCol.height * 1.25
                        radius:     ScreenTools.defaultFontPixelWidth * 0.5
                        color:      qgcPal.window
                        border.width:   1
                        border.color:   qgcPal.text
                        Keys.onBackPressed: {
                            passwordLoader.sourceComponent = null
                        }
                        MouseArea {
                            anchors.fill:   parent
                            onWheel:        { wheel.accepted = true; }
                            onPressed:      { mouse.accepted = true; }
                            onReleased:     { mouse.accepted = true; }
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
                                        var pwd = passwordField.text;
                                        passwordField.text = ""
                                        connectWifi(pwd)
                                    }
                                }
                                QGCButton {
                                    text:       qsTr("Cancel")
                                    width:      _labelWidth
                                    onClicked:  {
                                        Qt.inputMethod.hide();
                                        ssidGroup.current = null
                                        passwordField.text = ""
                                        passwordLoader.sourceComponent = null
                                    }
                                }
                            }
                        }
                        Component.onCompleted: {
                            passwordLoader.width  = pwdRect.width
                            passwordLoader.height = pwdRect.height
                        }
                    }
                }
            }
        }
    }
    
    Row {
        spacing:        ScreenTools.defaultFontPixelWidth
        anchors.top:    parent.top
        anchors.bottom: _isAP ? wifiLabel.top : parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        QGCColoredImage {
            width:              height
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            sourceSize.height:  height
            source:             "/typhoonh/img/wifi.svg"
            fillMode:           Image.PreserveAspectFit
            opacity:            _hasWifi ? 1 : 0.25
            color:              qgcPal.text
        }
        SignalStrength {
            opacity:                _hasWifi ? 1 : 0.25
            anchors.verticalCenter: parent.verticalCenter
            size:                   parent.height * 0.5
            percent: {
                if(TyphoonHQuickInterface.rssi < 0) {
                    if(TyphoonHQuickInterface.rssi >= -50)
                        return 100;
                    if(TyphoonHQuickInterface.rssi <= -100)
                        return 0;
                    return 2 * (TyphoonHQuickInterface.rssi + 100);
                }
                return 0;
            }
        }
    }
    QGCLabel {
        id:         wifiLabel
        text:       "AP Wi-Fi"
        visible:    _isAP
        color:      qgcPal.colorOrange
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
    }
    MouseArea {
        anchors.fill:   parent
        onClicked: {
            TyphoonHQuickInterface.wifiAlertEnabled = false
            if(_hasWifi) {
                var centerX = mapToItem(toolBar, x, y).x + (width / 2)
                mainWindow.showPopUp(wifiRSSIInfo, centerX)
            } else {
                rootLoader.sourceComponent = wifiManagement
                mainWindow.disableToolbar()
                if(mainWindow.currentPopUp) {
                    mainWindow.currentPopUp.close()
                }
            }
        }
    }
}

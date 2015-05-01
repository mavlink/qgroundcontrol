/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

/**
 * @file
 *   @brief QGC Main Tool Bar
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.Controls 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.MainToolBar 1.0
import QGroundControl.MavManager 1.0
import QGroundControl.ScreenTools 1.0

Rectangle {
    id: toolBarHolder

    property var qgcPal: QGCPalette { id: palette; colorGroupEnabled: true }

    property int cellSpacerSize: ScreenTools.isMobile ? getProportionalDimmension(6) : getProportionalDimmension(4)
    property int cellHeight:     getProportionalDimmension(30)

    property var colorBlue:       "#1a6eaa"
    property var colorGreen:      "#329147"
    property var colorRed:        "#942324"
    property var colorOrange:     "#a76f26"
    property var colorWhite:      "#f0f0f0"

    property var colorOrangeText: (qgcPal.globalTheme === QGCPalette.Light) ? "#b75711" : "#ea8225"
    property var colorRedText:    (qgcPal.globalTheme === QGCPalette.Light) ? "#ee1112" : "#ef2526"
    property var colorGreenText:  (qgcPal.globalTheme === QGCPalette.Light) ? "#046b1b" : "#00d930"
    property var colorWhiteText:  (qgcPal.globalTheme === QGCPalette.Light) ? "#343333" : "#f0f0f0"

    color:  qgcPal.windowShade

    function getProportionalDimmension(val) {
        return toolBarHolder.height * val / 40
    }

    function getMessageColor() {
        if(mainToolBar.messageType === MainToolBar.MessageNone)
            return qgcPal.button;
        if(mainToolBar.messageType === MainToolBar.MessageNormal)
            return colorBlue;
        if(mainToolBar.messageType === MainToolBar.MessageWarning)
            return colorOrange;
        if(mainToolBar.messageType === MainToolBar.MessageError)
            return colorRed;
        // Cannot be so make make it obnoxious to show error
        return "purple";
    }

    function getMessageIcon() {
        if(mainToolBar.messageType === MainToolBar.MessageNormal || mainToolBar.messageType === MainToolBar.MessageNone)
            return "qrc:/res/Megaphone";
        else
            return "qrc:/res/Yield";
    }

    function getBatteryIcon() {
        if(MavManager.batteryPercent < 20.0)
            return "qrc:/res/Battery_0";
        else if(MavManager.batteryPercent < 40.0)
            return "qrc:/res/Battery_20";
        else if(MavManager.batteryPercent < 60.0)
            return "qrc:/res/Battery_40";
        else if(MavManager.batteryPercent < 80.0)
            return "qrc:/res/Battery_60";
        else if(MavManager.batteryPercent < 90.0)
            return "qrc:/res/Battery_80";
        else
            return "qrc:/res/Battery_100";
    }

    function getBatteryColor() {
        if (MavManager.batteryPercent > 40.0)
            return colorGreen;
        if(MavManager.batteryPercent > 0.01)
            return colorRed;
        // This means there is no battery level data
        return colorBlue;
    }

    function getSatelliteColor() {
        // No GPS data
        if (MavManager.satelliteCount < 0)
            return qgcPal.button
        // No Lock
        if(MavManager.satelliteLock < 2)
            return colorRed;
        // 2D Lock
        if(MavManager.satelliteLock === 2)
            return colorBlue;
        // Lock is 3D or more
        return colorGreen;
    }

    function getRSSIColor(value) {
        if(value < 10)
            return colorRed;
        if(value < 50)
            return colorOrange;
        return colorGreen;
    }

    function showMavStatus() {
         return (MavManager.mavPresent && MavManager.heartbeatTimeout === 0 && mainToolBar.connectionCount > 0);
    }

    //-------------------------------------------------------------------------
    //-- Main menu for Mobile Devices
    Menu {
        id: maintMenu
        ExclusiveGroup { id: mainMenuGroup }
        MenuItem {
            text: "Vehicle Setup"
            checkable:  true
            exclusiveGroup: mainMenuGroup
            checked: (mainToolBar.currentView === MainToolBar.ViewSetup)
            onTriggered:
            {
                mainToolBar.onSetupView();
            }
        }
        MenuItem {
            text: "Plan View"
            checkable:  true
            checked: (mainToolBar.currentView === MainToolBar.ViewPlan)
            exclusiveGroup: mainMenuGroup
            onTriggered:
            {
                mainToolBar.onPlanView();
            }
        }
        MenuItem {
            text: "Flight View"
            checkable: true
            checked: (mainToolBar.currentView === MainToolBar.ViewFly)
            exclusiveGroup: mainMenuGroup
            onTriggered:
            {
                mainToolBar.onFlyView();
            }
        }
        //-- Flight View Context Menu
        MenuItem {
            text: "Flight View Options..."
            visible: (mainToolBar.currentView === MainToolBar.ViewFly)
            onTriggered:
            {
                mainToolBar.onFlyViewMenu();
            }
        }
    }

    Row {
        id:                     row1
        height:                 cellHeight
        anchors.left:           parent.left
        spacing:                getProportionalDimmension(4)
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin:     getProportionalDimmension(10)

        //---------------------------------------------------------------------
        //-- Main menu for Non Mobile Devices (Chevron Buttons)
        Row {
            id:                     row11
            height:                 cellHeight
            spacing:                -getProportionalDimmension(12)
            anchors.verticalCenter: parent.verticalCenter
            visible:                !ScreenTools.isMobile
            Connections {
                target: ScreenTools
                onRepaintRequestedChanged: {
                    setupButton.repaintChevron   = true;
                    planButton.repaintChevron    = true;
                    flyButton.repaintChevron     = true;
                    analyzeButton.repaintChevron = true;
                }
            }

            ExclusiveGroup { id: mainActionGroup }

            QGCToolBarButton {
                id: setupButton
                width: getProportionalDimmension(90)
                height: cellHeight
                exclusiveGroup: mainActionGroup
                text: qsTr("Setup")
                anchors.verticalCenter: parent.verticalCenter
                checked: (mainToolBar.currentView === MainToolBar.ViewSetup)
                onClicked: {
                    mainToolBar.onSetupView();
                }
                z: 1000
            }

            QGCToolBarButton {
                id: planButton
                width: getProportionalDimmension(90)
                height: cellHeight
                exclusiveGroup: mainActionGroup
                text: qsTr("Plan")
                anchors.verticalCenter: parent.verticalCenter
                checked: (mainToolBar.currentView === MainToolBar.ViewPlan)
                onClicked: {
                    mainToolBar.onPlanView();
                }
                z: 900
            }

            QGCToolBarButton {
                id: flyButton
                width: getProportionalDimmension(90)
                height: cellHeight
                exclusiveGroup: mainActionGroup
                text: qsTr("Fly")
                anchors.verticalCenter: parent.verticalCenter
                checked: (mainToolBar.currentView === MainToolBar.ViewFly)
                onClicked: {
                    mainToolBar.onFlyView();
                }
                z: 800
            }

            QGCToolBarButton {
                id: analyzeButton
                width: getProportionalDimmension(90)
                height: cellHeight
                exclusiveGroup: mainActionGroup
                text: qsTr("Analyze")
                anchors.verticalCenter: parent.verticalCenter
                checked: (mainToolBar.currentView === MainToolBar.ViewAnalyze)
                onClicked: {
                    mainToolBar.onAnalyzeView();
                }
                z: 700
            }

        }

        //---------------------------------------------------------------------
        //-- Indicators
        Row {
            id:                     row12
            height:                 cellHeight
            spacing:                cellSpacerSize
            anchors.verticalCenter: parent.verticalCenter

            //-- "Hamburger" menu for Mobile Devices
            Item {
                id:         actionButton
                visible:    ScreenTools.isMobile
                height:     cellHeight
                width:      cellHeight
                Image {
                    id:             buttomImg
                    anchors.fill:   parent
                    source:         "/qml/buttonMore.svg"
                    mipmap:         true
                    smooth:         true
                    antialiasing:   true
                    fillMode:       Image.PreserveAspectFit
                }
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onClicked: {
                        if (mouse.button == Qt.LeftButton)
                        {
                            maintMenu.popup();
                        }
                    }
                }
            }

            //-- Separator if Hamburger menu is visible
            Rectangle {
                visible:    actionButton.visible
                height:     cellHeight
                width:      cellHeight
                color:      "#00000000"
                anchors.verticalCenter: parent.verticalCenter
            }

            Rectangle {
                id: messages
                width: (mainToolBar.messageCount > 99) ? getProportionalDimmension(65) : getProportionalDimmension(60)
                height: cellHeight
                visible: (mainToolBar.connectionCount > 0) && (mainToolBar.showMessages)
                anchors.verticalCenter: parent.verticalCenter
                color:  getMessageColor()
                border.color: "#00000000"
                border.width: 0
                property bool showTriangle: false

                Image {
                    id: messageIcon
                    source: getMessageIcon();
                    height: getProportionalDimmension(16)
                    fillMode: Image.PreserveAspectFit
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: getProportionalDimmension(8)
                }

                Item {
                    id: messageTextRect
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    width: messages.width - messageIcon.width
                    QGCLabel {
                        id: messageText
                        text: (mainToolBar.messageCount > 0) ? mainToolBar.messageCount : ''
                        font.pointSize: ScreenTools.fontPointFactor * (14);
                        font.weight: Font.DemiBold
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.horizontalCenter: parent.horizontalCenter
                        horizontalAlignment: Text.AlignHCenter
                        color: colorWhite
                    }
                }

                Image {
                    id: dropDown
                    source: "QGroundControl/Controls/arrow-down.png"
                    visible: (messages.showTriangle) && (mainToolBar.messageCount > 0)
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    anchors.bottomMargin: getProportionalDimmension(3)
                    anchors.rightMargin:  getProportionalDimmension(3)
                }

                Timer {
                    id: mouseOffTimer
                    interval: 2000;
                    running: false;
                    repeat: false
                    onTriggered: {
                        messages.showTriangle = false;
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: {
                        messages.showTriangle = true;
                        mouseOffTimer.start();
                    }
                    onClicked: {
                        var p = mapToItem(toolBarHolder, mouseX, mouseY);
                        mainToolBar.onEnterMessageArea(p.x, p.y);
                    }
                }

            }

            Rectangle {
                id: mavIcon
                width: cellHeight
                height: cellHeight
                visible: showMavStatus() &&  (mainToolBar.showMav)
                anchors.verticalCenter: parent.verticalCenter
                color: colorBlue
                border.color: "#00000000"
                border.width: 0
                Image {
                    source: MavManager.systemPixmap
                    height: cellHeight * 0.75
                    fillMode: Image.PreserveAspectFit
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }

            Rectangle {
                id: satelitte
                width:  getProportionalDimmension(55)
                height: cellHeight
                visible: showMavStatus() && (mainToolBar.showGPS)
                anchors.verticalCenter: parent.verticalCenter
                color:  getSatelliteColor();
                border.color: "#00000000"
                border.width: 0

                Image {
                    source: "qrc:/res/Gps";
                    height: getProportionalDimmension(24)
                    fillMode: Image.PreserveAspectFit
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: getProportionalDimmension(6)
                    mipmap: true
                    smooth: true
                }

                QGCLabel {
                    id: satelitteText
                    text: MavManager.satelliteCount >= 0 ? MavManager.satelliteCount : 'NA'
                    font.pointSize: MavManager.satelliteCount >= 0 ? ScreenTools.fontPointFactor * (14) : ScreenTools.fontPointFactor * (10)
                    font.weight: Font.DemiBold
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: getProportionalDimmension(6)
                    horizontalAlignment: Text.AlignRight
                    color: colorWhite
                }
            }

            Rectangle {
                id: rssiRC
                width:  getProportionalDimmension(55)
                height: cellHeight
                visible: showMavStatus() && mainToolBar.showRSSI && mainToolBar.remoteRSSI <= 100
                anchors.verticalCenter: parent.verticalCenter
                color:  getRSSIColor(mainToolBar.remoteRSSI);
                border.color: "#00000000"
                border.width: 0
                Image {
                    source: "qrc:/res/AntennaRC";
                    width: cellHeight * 0.7
                    fillMode: Image.PreserveAspectFit
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: getProportionalDimmension(6)
                    mipmap: true
                    smooth: true
                }
                QGCLabel {
                    text: mainToolBar.remoteRSSI
                    anchors.right: parent.right
                    anchors.rightMargin: getProportionalDimmension(6)
                    anchors.verticalCenter: parent.verticalCenter
                    horizontalAlignment: Text.AlignRight
                    font.pointSize: ScreenTools.fontPointFactor * (12);
                    font.weight: Font.DemiBold
                    color: colorWhite
                }
            }

            Rectangle {
                id: rssiTelemetry
                width:  getProportionalDimmension(80)
                height: cellHeight
                visible: showMavStatus() && (mainToolBar.showRSSI) && ((mainToolBar.telemetryRRSSI > 0) && (mainToolBar.telemetryLRSSI > 0))
                anchors.verticalCenter: parent.verticalCenter
                color:  getRSSIColor(Math.min(mainToolBar.telemetryRRSSI,mainToolBar.telemetryLRSSI));
                border.color: "#00000000"
                border.width: 0
                Image {
                    source: "qrc:/res/AntennaT";
                    width: cellHeight * 0.7
                    fillMode: Image.PreserveAspectFit
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: getProportionalDimmension(6)
                    mipmap: true
                    smooth: true
                }
                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right:          parent.right
                    anchors.rightMargin:    getProportionalDimmension(6)
                    Row {
                        anchors.right: parent.right
                        QGCLabel {
                            text: 'R '
                            font.pointSize: ScreenTools.fontPointFactor * (11);
                            font.weight: Font.DemiBold
                            color: colorWhite
                        }
                        QGCLabel {
                            text: mainToolBar.telemetryRRSSI + 'dB'
                            width: getProportionalDimmension(30)
                            horizontalAlignment: Text.AlignRight
                            font.pointSize: ScreenTools.fontPointFactor * (11);
                            font.weight: Font.DemiBold
                            color: colorWhite
                        }
                    }
                    Row {
                        anchors.right: parent.right
                        QGCLabel {
                            text: 'L '
                            font.pointSize: ScreenTools.fontPointFactor * (11);
                            font.weight: Font.DemiBold
                            color: colorWhite
                        }
                        QGCLabel {
                            text: mainToolBar.telemetryLRSSI + 'dB'
                            width: getProportionalDimmension(30)
                            horizontalAlignment: Text.AlignRight
                            font.pointSize: ScreenTools.fontPointFactor * (11);
                            font.weight: Font.DemiBold
                            color: colorWhite
                        }
                    }
                }
            }

            Rectangle {
                id: battery
                width: getProportionalDimmension(60)
                height: cellHeight
                visible: showMavStatus() && (mainToolBar.showBattery)
                anchors.verticalCenter: parent.verticalCenter
                color:  getBatteryColor();
                border.color: "#00000000"
                border.width: 0

                Image {
                    source: getBatteryIcon();
                    height: getProportionalDimmension(20)
                    fillMode: Image.PreserveAspectFit
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: getProportionalDimmension(6)
                    mipmap: true
                    smooth: true
                }

                QGCLabel {
                    id: batteryText
                    text: MavManager.batteryVoltage.toFixed(1) + 'V';
                    font.pointSize: ScreenTools.fontPointFactor * (11);
                    font.weight: Font.DemiBold
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: getProportionalDimmension(6)
                    horizontalAlignment: Text.AlignRight
                    color: colorWhite
                }
            }

            Column {
                visible: showMavStatus()
                height:  cellHeight * 0.85
                width:   getProportionalDimmension(80)
                anchors.verticalCenter: parent.verticalCenter

                Rectangle {
                    id: armedStatus
                    width: parent.width
                    height: parent.height / 2
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: "#00000000"
                    border.color: "#00000000"
                    border.width: 0

                    QGCLabel {
                        id: armedStatusText
                        text: (MavManager.systemArmed) ? qsTr("ARMED") :  qsTr("DISARMED")
                        font.pointSize: ScreenTools.fontPointFactor * (12);
                        font.weight: Font.DemiBold
                        anchors.centerIn: parent
                        color: (MavManager.systemArmed) ? colorOrangeText : colorGreenText
                    }
                }

                Rectangle {
                    id: stateStatus
                    width: parent.width
                    height: parent.height / 2
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: "#00000000"
                    border.color: "#00000000"
                    border.width: 0

                    QGCLabel {
                        id: stateStatusText
                        text: MavManager.currentState
                        font.pointSize: ScreenTools.fontPointFactor * (12);
                        font.weight: Font.DemiBold
                        anchors.centerIn: parent
                        color: (MavManager.currentState === "STANDBY") ? colorGreenText : colorRedText
                    }
                }

            }

            Rectangle {
                id: modeStatus
                width: getProportionalDimmension(90)
                height: cellHeight
                visible: showMavStatus()
                color: "#00000000"
                border.color: "#00000000"
                border.width: 0

                QGCLabel {
                    id: modeStatusText
                    text: MavManager.currentMode
                    font.pointSize: ScreenTools.fontPointFactor * (12);
                    font.weight: Font.DemiBold
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    color: colorWhiteText
                }
            }

            Rectangle {
                id: connectionStatus
                width: getProportionalDimmension(160)
                height: cellHeight
                visible: (mainToolBar.connectionCount > 0 && MavManager.mavPresent && MavManager.heartbeatTimeout != 0)
                anchors.verticalCenter: parent.verticalCenter
                color: "#00000000"
                border.color: "#00000000"
                border.width: 0

                QGCLabel {
                    id: connectionStatusText
                    text: qsTr("CONNECTION LOST")
                    font.pointSize: ScreenTools.fontPointFactor * (14);
                    font.weight: Font.DemiBold
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: colorRedText
                }
            }
        }
    }

    Row {
        id: row2
        height: cellHeight
        spacing: cellSpacerSize
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin:  getProportionalDimmension(10)
        anchors.rightMargin: getProportionalDimmension(10)

        Menu {
            id: connectMenu
            Component.onCompleted: {
                mainToolBar.configListChanged.connect(connectMenu.updateConnectionList);
                connectMenu.updateConnectionList();
            }
            function addMenuEntry(name) {
                var label = "Add Connection"
                if(name !== "")
                    label = name;
                var mItem = connectMenu.addItem(label);
                var menuSlot = function() {mainToolBar.onConnect(name)};
                mItem.triggered.connect(menuSlot);
            }
            function updateConnectionList() {
                connectMenu.clear();
                for(var i = 0; i < mainToolBar.configList.length; i++) {
                    connectMenu.addMenuEntry(mainToolBar.configList[i]);
                }
                if(mainToolBar.configList.length > 0) {
                    connectMenu.addSeparator();
                }
                // Add "Add Connection" to the list
                connectMenu.addMenuEntry("");
            }
        }

        QGCButton {
            id:         connectButton
            width:      getProportionalDimmension(100)
            visible:    mainToolBar.connectionCount === 0
            text:       qsTr("Connect")
            menu:       connectMenu
            anchors.verticalCenter: parent.verticalCenter
        }

        QGCButton {
            id:         disconnectButton
            width:      getProportionalDimmension(100)
            visible:    mainToolBar.connectionCount === 1
            text:       qsTr("Disconnect")
            anchors.verticalCenter: parent.verticalCenter
            onClicked: {
                mainToolBar.onDisconnect("");
            }
        }

        Menu {
            id: disconnectMenu
            Component.onCompleted: {
                mainToolBar.connectedListChanged.connect(disconnectMenu.onConnectedListChanged)
            }
            function addMenuEntry(name) {
                var mItem = disconnectMenu.addItem(name);
                var menuSlot = function() {mainToolBar.onDisconnect(name)};
                mItem.triggered.connect(menuSlot);
            }
            function onConnectedListChanged(conList) {
                disconnectMenu.clear();
                for(var i = 0; i < conList.length; i++) {
                    disconnectMenu.addMenuEntry(conList[i]);
                }
            }
        }

        QGCButton {
            id:         multidisconnectButton
            width:      getProportionalDimmension(100)
            text:       "Disconnect"
            visible:    mainToolBar.connectionCount > 1
            menu:       disconnectMenu
            anchors.verticalCenter: parent.verticalCenter
        }

    }

    // Progress bar
    Rectangle {
        readonly property int progressBarHeight: getProportionalDimmension(3)
        y:      parent.height  - progressBarHeight
        height: progressBarHeight
        width:  parent.width * mainToolBar.progressBarValue
        color:  qgcPal.text
    }
}


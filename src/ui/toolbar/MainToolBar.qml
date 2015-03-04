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

Rectangle {

    property var qgcPal: QGCPalette { id: palette; colorGroupEnabled: true }
    property int cellSpacerSize: 4
    property int cellHeight:     30
    property int cellRadius:     3
    property double dpiFactor: (72.0 / mainToolBar.dotsPerInch);

    property var colorBlue:       "#1a6eaa"
    property var colorGreen:      "#079527"
    property var colorGreenText:  "#00d930"
    property var colorRed:        "#a81a1b"
    property var colorOrange:     "#a76f26"
    property var colorWhite:      "#f0f0f0"

    id: toolBarHolder
    color: qgcPal.windowShade

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
            return "qrc:/files/images/status/message_megaphone.png";
        else
            return "qrc:/files/images/status/message_triangle.png";
    }

    function getBatteryIcon() {
        if(mainToolBar.batteryPercent < 20.0)
            return "qrc:/files/images/status/battery_0.svg";
        else if(mainToolBar.batteryPercent < 40.0)
            return "qrc:/files/images/status/battery_20.svg";
        else if(mainToolBar.batteryPercent < 60.0)
            return "qrc:/files/images/status/battery_40.svg";
        else if(mainToolBar.batteryPercent < 80.0)
            return "qrc:/files/images/status/battery_60.svg";
        else if(mainToolBar.batteryPercent < 90.0)
            return "qrc:/files/images/status/battery_80.svg";
        else
            return "qrc:/files/images/status/battery_100.svg";
    }

    function getBatteryColor() {
        if (mainToolBar.batteryPercent > 40.0)
            return colorGreen;
        if(mainToolBar.batteryPercent > 0.01)
            return colorRed;
        // This means there is no battery level data
        return colorBlue;
    }

    function getSatelliteColor() {
        // No GPS data
        if (mainToolBar.satelliteCount < 0)
            return qgcPal.button
        // No Lock
        if(mainToolBar.satelliteLock < 2)
            return colorRed;
        // 2D Lock
        if(mainToolBar.satelliteLock === 2)
            return colorBlue;
        // Lock is 3D or more
        return colorGreen;
    }

    function showMavStatus() {
         return (mainToolBar.mavPresent && mainToolBar.heartbeatTimeout === 0 && mainToolBar.connectionCount > 0);
    }

    Row {
        id:                     row1
        height:                 cellHeight
        anchors.left:           parent.left
        spacing:                4
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin:     10

        Row {
            id:                     row11
            height:                 cellHeight
            spacing:                -12
            anchors.verticalCenter: parent.verticalCenter

            Connections {
                target: mainToolBar
                onRepaintRequestedChanged: {
                    setupButton.repaintChevron = true;
                    planButton.repaintChevron = true;
                    flyButton.repaintChevron = true;
                    analyzeButton.repaintChevron = true;
                }
            }

            ExclusiveGroup { id: mainActionGroup }

            QGCToolBarButton {
                id: setupButton
                width: 100
                height: cellHeight
                exclusiveGroup: mainActionGroup
                text: qsTr("1. Setup")
                anchors.verticalCenter: parent.verticalCenter
                checked: (mainToolBar.currentView === MainToolBar.ViewSetup)
                onClicked: {
                    mainToolBar.onSetupView();
                }
                z: 1000
            }

            QGCToolBarButton {
                id: planButton
                width: 100
                height: cellHeight
                exclusiveGroup: mainActionGroup
                text: qsTr("2. Plan")
                anchors.verticalCenter: parent.verticalCenter
                checked: (mainToolBar.currentView === MainToolBar.ViewPlan)
                onClicked: {
                    mainToolBar.onPlanView();
                }
                z: 900
            }

            QGCToolBarButton {
                id: flyButton
                width: 100
                height: cellHeight
                exclusiveGroup: mainActionGroup
                text: qsTr("3. Fly")
                anchors.verticalCenter: parent.verticalCenter
                checked: (mainToolBar.currentView === MainToolBar.ViewFly)
                onClicked: {
                    mainToolBar.onFlyView();
                }
                z: 800
            }

            QGCToolBarButton {
                id: analyzeButton
                width: 100
                height: cellHeight
                exclusiveGroup: mainActionGroup
                text: qsTr("4. Analyze")
                anchors.verticalCenter: parent.verticalCenter
                checked: (mainToolBar.currentView === MainToolBar.ViewAnalyze)
                onClicked: {
                    mainToolBar.onAnalyzeView();
                }
                z: 700
            }

        }

        Row {
            id:                     row12
            height:                 cellHeight
            spacing:                cellSpacerSize
            anchors.verticalCenter: parent.verticalCenter

            Rectangle {
                id: messages
                width: (mainToolBar.messageCount > 99) ? 70 : 60
                height: cellHeight
                visible: (mainToolBar.connectionCount > 0) && (mainToolBar.showMessages)
                anchors.verticalCenter: parent.verticalCenter
                color:  getMessageColor()
                radius: cellRadius
                border.color: "#00000000"
                border.width: 0
                property bool showTriangle: false

                Image {
                    id: messageIcon
                    source: getMessageIcon();
                    height: 16
                    fillMode: Image.PreserveAspectFit
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                }

                Rectangle {
                    id: messageTextRect
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    width: messages.width - messageIcon.width
                    Text {
                        id: messageText
                        text: (mainToolBar.messageCount > 0) ? mainToolBar.messageCount : ''
                        font.pointSize: 14 * dpiFactor
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
                    anchors.bottomMargin: 3
                    anchors.rightMargin: 3
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
                radius: cellRadius
                border.color: "#00000000"
                border.width: 0
                Image {
                    source: mainToolBar.systemPixmap
                    height: cellHeight * 0.75
                    fillMode: Image.PreserveAspectFit
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }

            Rectangle {
                id: satelitte
                width: 60
                height: cellHeight
                visible: showMavStatus() && (mainToolBar.showGPS)
                anchors.verticalCenter: parent.verticalCenter
                color:  getSatelliteColor();
                radius: cellRadius
                border.color: "#00000000"
                border.width: 0

                Image {
                    source: "qrc:/files/images/status/gps.svg";
                    height: 24
                    fillMode: Image.PreserveAspectFit
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    mipmap: true
                    smooth: true
                }

                Text {
                    id: satelitteText
                    text: (mainToolBar.satelliteCount > 0) ? mainToolBar.satelliteCount : ''
                    font.pointSize: 14 * dpiFactor
                    font.weight: Font.DemiBold
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: 10
                    horizontalAlignment: Text.AlignRight
                    color: colorWhite
                }
            }

            Rectangle {
                id: battery
                width: 80
                height: cellHeight
                visible: showMavStatus() && (mainToolBar.showBattery)
                anchors.verticalCenter: parent.verticalCenter
                color:  (mainToolBar.batteryPercent > 40.0 || mainToolBar.batteryPercent < 0.01) ? colorBlue : colorRed
                radius: cellRadius
                border.color: "#00000000"
                border.width: 0

                Image {
                    source: getBatteryIcon();
                    height: 20
                    fillMode: Image.PreserveAspectFit
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 6
                    mipmap: true
                    smooth: true
                }

                Text {
                    id: batteryText
                    text: mainToolBar.batteryVoltage.toFixed(1) + ' V';
                    font.pointSize: 14 * dpiFactor
                    font.weight: Font.DemiBold
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: 8
                    horizontalAlignment: Text.AlignRight
                    color: colorWhite
                }
            }

            Column {
                visible: showMavStatus()
                height:  cellHeight * 0.85
                width:   80
                anchors.verticalCenter: parent.verticalCenter

                Rectangle {
                    id: armedStatus
                    width: parent.width
                    height: parent.height / 2
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: "#00000000"
                    border.color: "#00000000"
                    border.width: 0

                    Text {
                        id: armedStatusText
                        text: (mainToolBar.systemArmed) ? qsTr("ARMED") :  qsTr("DISARMED")
                        font.pointSize: 12 * dpiFactor
                        font.weight: Font.DemiBold
                        anchors.centerIn: parent
                        color: (mainToolBar.systemArmed) ? colorRed : colorGreen
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

                    Text {
                        id: stateStatusText
                        text: mainToolBar.currentState
                        font.pointSize: 12 * dpiFactor
                        font.weight: Font.DemiBold
                        anchors.centerIn: parent
                        color: (mainToolBar.currentState === "STANDBY") ? colorGreen : colorRed
                    }
                }

            }

            Rectangle {
                id: modeStatus
                width: 90
                height: cellHeight
                visible: showMavStatus()
                color: "#00000000"
                border.color: "#00000000"
                border.width: 0

                Text {
                    id: modeStatusText
                    text: mainToolBar.currentMode
                    font.pointSize: 12 * dpiFactor
                    font.weight: Font.DemiBold
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    color: qgcPal.text
                }
            }

            Rectangle {
                id: connectionStatus
                width: 160
                height: cellHeight
                visible: (mainToolBar.connectionCount > 0 && mainToolBar.mavPresent && mainToolBar.heartbeatTimeout != 0)
                anchors.verticalCenter: parent.verticalCenter
                color: "#00000000"
                border.color: "#00000000"
                border.width: 0

                Text {
                    id: connectionStatusText
                    text: qsTr("CONNECTION LOST")
                    font.pointSize: 14 * dpiFactor
                    font.weight: Font.DemiBold
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: colorRed
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
        anchors.leftMargin:  10
        anchors.rightMargin: 10

        QGCComboBox {
            id: configList
            width: 200
            height: cellHeight
            visible: (mainToolBar.connectionCount === 0 && mainToolBar.configList.length > 0)
            anchors.verticalCenter: parent.verticalCenter
            model: mainToolBar.configList
            onCurrentIndexChanged: {
                mainToolBar.onLinkConfigurationChanged(mainToolBar.configList[currentIndex]);
            }
            Component.onCompleted: {
                mainToolBar.currentConfigChanged.connect(configList.onCurrentConfigChanged)
            }
            function onCurrentConfigChanged(config) {
                var index = configList.find(config);
                configList.currentIndex = index;
            }
        }

        QGCButton {
            id: connectButton
            width: 100
            height: cellHeight
            visible: (mainToolBar.connectionCount === 0 || mainToolBar.connectionCount === 1)
            text: (mainToolBar.configList.length > 0) ? (mainToolBar.connectionCount === 0) ? qsTr("Connect") : qsTr("Disconnect") : qsTr("Add Link")
            anchors.verticalCenter: parent.verticalCenter
            onClicked: {
                mainToolBar.onConnect("");
            }
        }

        Menu {
            id: disconnectMenu
            Component.onCompleted: {
                mainToolBar.connectedListChanged.connect(disconnectMenu.onConnectedListChanged)
            }
            function onConnectedListChanged(conList) {
                disconnectMenu.clear();
                for(var i = 0; i < conList.length; i++) {
                    var mItem = disconnectMenu.addItem(conList[i]);
                    var menuSlot = function() {mainToolBar.onConnect(mItem.text)};
                    mItem.triggered.connect(menuSlot);
                }
            }
        }

        QGCButton {
            id: multidisconnectButton
            width: 100
            height: cellHeight
            text: qsTr("Disconnect")
            visible: (mainToolBar.connectionCount > 1)
            anchors.verticalCenter: parent.verticalCenter
            menu: disconnectMenu
        }

    }
}


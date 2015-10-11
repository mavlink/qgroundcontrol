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

import QGroundControl.Controls              1.0
import QGroundControl.FactControls          1.0
import QGroundControl.Palette               1.0
import QGroundControl.MainToolBar           1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0

Rectangle {
    id: toolBarHolder

    property var qgcPal: QGCPalette { id: palette; colorGroupEnabled: true }

    property var activeVehicle: multiVehicleManager.activeVehicle

    readonly property real toolBarHeight:   ScreenTools.defaultFontPixelHeight * 3
    property int cellSpacerSize:            ScreenTools.isMobile ? getProportionalDimmension(6) : getProportionalDimmension(4)
    readonly property int cellHeight:       toolBarHeight * 0.75

    readonly property real horizontalMargins:   ScreenTools.defaultFontPixelWidth / 2
    readonly property real verticalMargins:     ScreenTools.defaultFontPixelHeight / 4

    readonly property var colorBlue:    "#1a6eaa"
    readonly property var colorGreen:   "#329147"
    readonly property var colorRed:     "#942324"
    readonly property var colorOrange:  "#a76f26"
    readonly property var colorWhite:   "#f0f0f0"

    property var colorOrangeText: (qgcPal.globalTheme === QGCPalette.Light) ? "#b75711" : "#ea8225"
    property var colorRedText:    (qgcPal.globalTheme === QGCPalette.Light) ? "#ee1112" : "#ef2526"
    property var colorGreenText:  (qgcPal.globalTheme === QGCPalette.Light) ? "#046b1b" : "#00d930"
    property var colorWhiteText:  (qgcPal.globalTheme === QGCPalette.Light) ? "#343333" : "#f0f0f0"

    color:  qgcPal.windowShade

    Connections {
        target: mainToolBar

        onShowMessage: {
            toolBarMessage.text = message
            if (toolBarMessage.contentHeight > toolBarMessageCloseButton.height) {
                mainToolBar.height = toolBarHeight + toolBarMessage.contentHeight + (verticalMargins * 2)
            } else {
                mainToolBar.height = toolBarHeight + toolBarMessageCloseButton.height + (verticalMargins * 2)
            }
            toolBarMessageArea.visible = true
        }
    }

    function getProportionalDimmension(val) {
        return toolBarHeight * val / 40
    }

    function getMessageColor() {
        if (activeVehicle.messageTypeNone)
            return qgcPal.button;
        if (activeVehicle.messageTypeNorma)
            return colorBlue;
        if (activeVehicle.messageTypeWarning)
            return colorOrange;
        if (activeVehicle.messageTypeError)
            return colorRed;
        // Cannot be so make make it obnoxious to show error
        return "purple";
    }

    function getMessageIcon() {
        if (activeVehicle.messageTypeNormal || activeVehicle.messageTypeNone)
            return "qrc:/res/Megaphone";
        else
            return "qrc:/res/Yield";
    }

    function getBatteryIcon() {
        if(activeVehicle.batteryPercent < 20.0)
            return "qrc:/res/Battery_0";
        else if(activeVehicle.batteryPercent < 40.0)
            return "qrc:/res/Battery_20";
        else if(activeVehicle.batteryPercent < 60.0)
            return "qrc:/res/Battery_40";
        else if(activeVehicle.batteryPercent < 80.0)
            return "qrc:/res/Battery_60";
        else if(activeVehicle.batteryPercent < 90.0)
            return "qrc:/res/Battery_80";
        else
            return "qrc:/res/Battery_100";
    }

    function getBatteryColor() {
        if (activeVehicle.batteryPercent > 40.0)
            return colorGreen;
        if(activeVehicle.batteryPercent > 0.01)
            return colorRed;
        // This means there is no battery level data
        return colorBlue;
    }

    function getSatelliteColor() {
        // No GPS data
        if (activeVehicle.satelliteCount < 0)
            return qgcPal.button
        // No Lock
        if(activeVehicle.satelliteLock < 2)
            return colorRed;
        // 2D Lock
        if(activeVehicle.satelliteLock === 2)
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
         return (multiVehicleManager.activeVehicleAvailable && activeVehicle.heartbeatTimeout === 0 && mainToolBar.connectionCount > 0);
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
    } // Menu

    Component {
        id: activeVehicleComponent

        Row {
            height:     cellHeight
            spacing:    cellSpacerSize

            Rectangle {
                id: messages
                width: (activeVehicle.messageCount > 99) ? getProportionalDimmension(65) : getProportionalDimmension(60)
                height: cellHeight
                visible: mainToolBar.showMessages
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
                        text: (activeVehicle.messageCount > 0) ? activeVehicle.messageCount : ''
                        font.pixelSize: ScreenTools.smallFontPixelSize
                        font.weight: Font.DemiBold
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.horizontalCenter: parent.horizontalCenter
                        horizontalAlignment: Text.AlignHCenter
                        color: colorWhite
                    }
                }

                Image {
                    id: dropDown
                    source: "/qmlimages/arrow-down.png"
                    visible: (messages.showTriangle) && (activeVehicle.messageCount > 0)
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

            QGCButton {
                width:                  ScreenTools.defaultFontPixelWidth * 12
                height:                 cellHeight
                visible:                mainToolBar.showMav
                anchors.verticalCenter: parent.verticalCenter
                text:                   "Vehicle " + activeVehicle.id

                menu: vehicleMenu

                Menu {
                    id: vehicleMenu
                }

                Component {
                    id: vehicleMenuItemComponent

                    MenuItem {
                        checkable:      true
                        checked:        vehicle.active
                        onTriggered:    multiVehicleManager.activeVehicle = vehicle

                        property int vehicleId: Number(text.split(" ")[1])
                        property var vehicle:   multiVehicleManager.getVehicleById(vehicleId)
                    }
                }

                property var vehicleMenuItems: []

                function updateVehicleMenu() {
                    // Remove old menu items
                    for (var i=0; i<vehicleMenuItems.length; i++) {
                        vehicleMenu.removeItem(vehicleMenuItems[i])
                    }
                    vehicleMenuItems.length = 0

                    // Add new items
                    for (var i=0; i<multiVehicleManager.vehicles.count; i++) {
                        var vehicle = multiVehicleManager.vehicles.get(i)
                        var menuItem = vehicleMenuItemComponent.createObject(null, { "text": "Vehicle " + vehicle.id })
                        vehicleMenuItems.push(menuItem)
                        vehicleMenu.insertItem(i, menuItem)
                    }
                }

                Component.onCompleted: updateVehicleMenu()

                Connections {
                    target:         multiVehicleManager.vehicles
                    onCountChanged: parent.updateVehicleMenu
                }
            }


            Rectangle {
                id: satelitte
                width:  getProportionalDimmension(55)
                height: cellHeight
                visible: mainToolBar.showGPS
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
                    text: activeVehicle.satelliteCount >= 0 ? activeVehicle.satelliteCount : 'NA'
                    font.pixelSize: activeVehicle.satelliteCount >= 0 ? ScreenTools.defaultFontPixelSize : ScreenTools.smallFontPixelSize
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
                visible: mainToolBar.showRSSI && mainToolBar.remoteRSSI <= 100
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
                    font.pixelSize: ScreenTools.smallFontPixelSize
                    font.weight: Font.DemiBold
                    color: colorWhite
                }
            }

            Rectangle {
                id: rssiTelemetry
                width:  getProportionalDimmension(80)
                height: cellHeight
                visible: mainToolBar.showRSSI && (mainToolBar.telemetryRRSSI > 0) && (mainToolBar.telemetryLRSSI > 0)
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
                            font.pixelSize: ScreenTools.smallFontPixelSize
                            font.weight: Font.DemiBold
                            color: colorWhite
                        }
                        QGCLabel {
                            text: mainToolBar.telemetryRRSSI + 'dB'
                            width: getProportionalDimmension(30)
                            horizontalAlignment: Text.AlignRight
                            font.pixelSize: ScreenTools.smallFontPixelSize
                            font.weight: Font.DemiBold
                            color: colorWhite
                        }
                    }
                    Row {
                        anchors.right: parent.right
                        QGCLabel {
                            text: 'L '
                            font.pixelSize: ScreenTools.smallFontPixelSize
                            font.weight: Font.DemiBold
                            color: colorWhite
                        }
                        QGCLabel {
                            text: mainToolBar.telemetryLRSSI + 'dB'
                            width: getProportionalDimmension(30)
                            horizontalAlignment: Text.AlignRight
                            font.pixelSize: ScreenTools.smallFontPixelSize
                            font.weight: Font.DemiBold
                            color: colorWhite
                        }
                    }
                }
            }

            Rectangle {
                id: batteryStatus
                width:  activeVehicle.batteryConsumed < 0.0 ? getProportionalDimmension(60) : getProportionalDimmension(80)
                height: cellHeight
                visible: mainToolBar.showBattery
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
                    visible: batteryStatus.visible && activeVehicle.batteryConsumed < 0.0
                    text: (activeVehicle.batteryVoltage > 0) ? activeVehicle.batteryVoltage.toFixed(1) + 'V' : '---';
                    font.pixelSize: ScreenTools.smallFontPixelSize
                    font.weight: Font.DemiBold
                    anchors.right: parent.right
                    anchors.rightMargin: getProportionalDimmension(6)
                    anchors.verticalCenter: parent.verticalCenter
                    horizontalAlignment: Text.AlignRight
                    color: colorWhite
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right:          parent.right
                    anchors.rightMargin:    getProportionalDimmension(6)
                    visible: batteryStatus.visible && activeVehicle.batteryConsumed >= 0.0
                    QGCLabel {
                        text: (activeVehicle.batteryVoltage > 0) ? activeVehicle.batteryVoltage.toFixed(1) + 'V' : '---';
                        width: getProportionalDimmension(30)
                        horizontalAlignment: Text.AlignRight
                        font.pixelSize: ScreenTools.smallFontPixelSize
                        font.weight: Font.DemiBold
                        color: colorWhite
                    }
                    QGCLabel {
                        text: (activeVehicle.batteryConsumed > 0) ? activeVehicle.batteryConsumed.toFixed(0) + 'mAh' : '---';
                        width: getProportionalDimmension(30)
                        horizontalAlignment: Text.AlignRight
                        font.pixelSize: ScreenTools.smallFontPixelSize
                        font.weight: Font.DemiBold
                        color: colorWhite
                    }
                }
            }

            QGCButton {
                width:                  ScreenTools.defaultFontPixelWidth * 11
                height:                 cellHeight
                anchors.verticalCenter: parent.verticalCenter
                text:                   activeVehicle.armed ? "Armed" : "Disarmed"

                menu: Menu {
                    MenuItem {
                        enabled: !activeVehicle.armed
                        text: "Arm"

                        onTriggered: activeVehicle.armed = true
                    }

                    MenuItem {
                        enabled: activeVehicle.armed
                        text: "Disarm"

                        onTriggered: activeVehicle.armed = false
                    }
                }
            }

            QGCButton {
                width:                  ScreenTools.defaultFontPixelWidth * 15
                height:                 cellHeight
                anchors.verticalCenter: parent.verticalCenter
                text:                   activeVehicle.flightMode

                menu: activeVehicle.flightModeSetAvailable ? flightModesMenu : null

                Menu {
                    id: flightModesMenu
                }

                Component {
                    id: flightModeMenuItemComponent

                    MenuItem {
                        checkable:      true
                        checked:        activeVehicle.flightMode == text
                        onTriggered:    activeVehicle.flightMode = text
                    }
                }

                property var flightModesMenuItems: []

                function updateFlightModesMenu() {
                    if (activeVehicle.flightModeSetAvailable) {
                        // Remove old menu items
                        for (var i=0; i<flightModesMenuItems.length; i++) {
                            flightModesMenu.removeItem(flightModesMenuItems[i])
                        }
                        flightModesMenuItems.length = 0

                            // Add new items
                            for (var i=0; i<activeVehicle.flightModes.length; i++) {
                                var menuItem = flightModeMenuItemComponent.createObject(null, { "text": activeVehicle.flightModes[i] })
                                flightModesMenuItems.push(menuItem)
                                flightModesMenu.insertItem(i, menuItem)
                            }
                    }
                }

                Component.onCompleted: updateFlightModesMenu()

                Connections {
                    target:                 multiVehicleManager
                    onActiveVehicleChanged: parent.updateFlightModesMenu
                }
            }

            Rectangle {
                width:                  ScreenTools.defaultFontPixelWidth * 4
                height:                 cellHeight
                anchors.verticalCenter: parent.verticalCenter
                color:                  colorBlue
                border.width:           0
                visible:                activeVehicle.hilMode

                QGCLabel {
                    anchors.fill:           parent
                    horizontalAlignment:    Text.AlignHCenter
                    verticalAlignment:      Text.AlignVCenter
                    text:                   "HIL"
                }
            }

        } // Row
    } // Component - activeVehicleComponent

    Row {
        id:         toolRow
        x:          horizontalMargins
        y:          (toolBarHeight - cellHeight) / 2
        height:     cellHeight
        spacing:    getProportionalDimmension(4)

        //---------------------------------------------------------------------
        //-- Main menu for Non Mobile Devices (Chevron Buttons)
        Row {
            id:             row11
            height:         cellHeight
            spacing:        -getProportionalDimmension(12)
            anchors.top:    parent.top
            visible:        !ScreenTools.isMobile

            Connections {
                target: ScreenTools
                onRepaintRequested: {
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
                checked: (mainToolBar.currentView === MainToolBar.ViewAnalyze)
                onClicked: {
                    mainToolBar.onAnalyzeView();
                }
                z: 700
            }
        } // Row

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
                    source:         "/qmlimages/buttonMore.svg"
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

            Loader {
                id:                 activeVehicleLoader
                visible:            showMavStatus()
                sourceComponent:    multiVehicleManager.activeVehicleAvailable ? activeVehicleComponent : undefined

                property real cellHeight:       toolBarHolder.cellHeight
                property real cellSpacerSize:   toolBarHolder.cellSpacerSize
            }

            Rectangle {
                id: connectionStatus
                width: getProportionalDimmension(160)
                height: cellHeight
                visible: (mainToolBar.connectionCount > 0 && multiVehicleManager.activeVehicleAvailable && activeVehicle.heartbeatTimeout != 0)
                anchors.verticalCenter: parent.verticalCenter
                color: "#00000000"
                border.color: "#00000000"
                border.width: 0

                QGCLabel {
                    id: connectionStatusText
                    text: qsTr("CONNECTION LOST")
                    font.pixelSize: ScreenTools.defaultFontPixelSize
                    font.weight: Font.DemiBold
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: colorRedText
                }
            }
        } // Row
    } // Row

    Row {
        id:                     connectRow
        anchors.rightMargin:    verticalMargins
        anchors.right:          parent.right
        anchors.top:            toolRow.top
        anchors.verticalCenter: toolRow.verticalCenter
        height:                 toolRow.height
        spacing:                cellSpacerSize

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
        }

        QGCButton {
            id:         disconnectButton
            width:      getProportionalDimmension(100)
            visible:    mainToolBar.connectionCount === 1
            text:       qsTr("Disconnect")
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
        }
    } // Row

    // Progress bar
    Rectangle {
        id:             progressBar
        anchors.top:    toolRow.bottom
        height:         getProportionalDimmension(3)
        width:          parent.width * mainToolBar.progressBarValue
        color:          qgcPal.text
    }

    // Toolbar message area
    Rectangle {
        id:                     toolBarMessageArea
        anchors.leftMargin:     horizontalMargins
        anchors.rightMargin:    horizontalMargins
        anchors.topMargin:      verticalMargins
        anchors.bottomMargin:   verticalMargins
        anchors.top:            progressBar.bottom
        anchors.bottom:         parent.bottom
        anchors.left:           parent.left
        anchors.right:          parent.right
        color:                  qgcPal.windowShadeDark
        visible:                false

        QGCLabel {
            id:             toolBarMessage
            anchors.fill:   parent
            wrapMode:       Text.WordWrap
			color:			qgcPal.warningText
        }

        QGCButton {
            id:                     toolBarMessageCloseButton
            anchors.rightMargin:    horizontalMargins
            anchors.top:            parent.top
            anchors.right:          parent.right
			primary:				true
            text:                   "Close Message"

            onClicked: {
                parent.visible = false
                mainToolBar.height = toolBarHeight
                mainToolBar.onToolBarMessageClosed()
            }
        }
    }
} // Rectangle
